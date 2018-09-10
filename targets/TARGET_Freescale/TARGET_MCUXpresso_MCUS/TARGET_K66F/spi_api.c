/* mbed Microcontroller Library
 * Copyright (c) 2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <math.h>
#include "mbed_assert.h"

#include "spi_api.h"

#if DEVICE_SPI

#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"
#include "fsl_dspi.h"
#include "peripheral_clock_defines.h"
#include "PeripheralPins.h"
#include "device.h"

/* Array of SPI peripheral base address. */
static SPI_Type *const spi_address[] = SPI_BASE_PTRS;
/* Array of SPI bus clock frequencies */
static clock_name_t const spi_clocks[] = SPI_CLOCK_FREQS;

SPIName spi_get_module(PinName mosi, PinName miso, PinName sclk) {
    int32_t spi_mosi = pinmap_find_peripheral(mosi, PinMap_SPI_SOUT);
    int32_t spi_miso = pinmap_find_peripheral(miso, PinMap_SPI_SIN);
    if ((spi_mosi == NC) && (spi_miso == NC)) {
        // we're probably in slave mode.
        spi_mosi = pinmap_peripheral(mosi, PinMap_SPI_SIN);
        spi_miso = pinmap_peripheral(miso, PinMap_SPI_SOUT);
    }
    int32_t spi_sclk = pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    int32_t spi_data = pinmap_merge(spi_mosi, spi_miso);

    return pinmap_merge(spi_data, spi_sclk);
}

void spi_init(spi_t *obj, bool is_slave, PinName mosi, PinName miso, PinName sclk, PinName ssel)
{
    // determine the SPI to use
    int32_t spi_module = (uint32_t)spi_get_module(mosi, miso, sclk);
    int32_t spi_ssel = pinmap_peripheral(ssel, PinMap_SPI_SSEL);

    obj->instance = pinmap_merge(spi_module, spi_ssel);
    MBED_ASSERT((int)obj->instance != NC);

    // pin out the spi pins
    if (!is_slave) {
        pinmap_pinout(mosi, PinMap_SPI_SOUT);
    } else {
        pinmap_pinout(mosi, PinMap_SPI_SIN);
    }
    if (!is_slave) {
        pinmap_pinout(miso, PinMap_SPI_SIN);
    } else {
        pinmap_pinout(miso, PinMap_SPI_SOUT);
    }
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    if (ssel != NC) {
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
    }
    obj->slave = is_slave;
}

void spi_free(spi_t *obj)
{
    DSPI_Deinit(spi_address[obj->instance]);
}

void spi_format(spi_t *obj, uint8_t bits, spi_mode_t mode, spi_bit_ordering_t bit_ordering)
{

    dspi_master_config_t master_config;
    dspi_slave_config_t slave_config;
    dspi_clock_polarity_t cpol;
    dspi_clock_phase_t cpha;

    if ((mode == SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE) ||
        (mode == SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE)) {
            cpol = kDSPI_ClockPolarityActiveLow;
    } else {
            cpol = kDSPI_ClockPolarityActiveHigh;
    }
    if ((mode == SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE) ||
        (mode == SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE)) {
            cpha = kDSPI_ClockPhaseFirstEdge;
    } else {
            cpha = kDSPI_ClockPhaseSecondEdge;
    }

    /* Bits: values between 4 and 16 are valid */
    MBED_ASSERT(bits >= 4 && bits <= 16);
    obj->bits = bits;
    obj->order = bit_ordering;

    if (obj->slave) {
        /* Slave config */
        DSPI_SlaveGetDefaultConfig(&slave_config);
        slave_config.whichCtar = kDSPI_Ctar0;
        slave_config.ctarConfig.bitsPerFrame = (uint32_t)bits;;
        slave_config.ctarConfig.cpol = cpol;
        slave_config.ctarConfig.cpha = cpha;

        DSPI_SlaveInit(spi_address[obj->instance], &slave_config);
    } else {
        /* Master config */
        DSPI_MasterGetDefaultConfig(&master_config);
        master_config.ctarConfig.bitsPerFrame = (uint32_t)bits;;
        master_config.ctarConfig.cpol = cpol;
        master_config.ctarConfig.cpha = cpha;
        master_config.ctarConfig.direction = (bit_ordering == SPI_BIT_ORDERING_MSB_FIRST)? kDSPI_MsbFirst : kDSPI_LsbFirst;
        master_config.ctarConfig.pcsToSckDelayInNanoSec = 0;

        DSPI_MasterInit(spi_address[obj->instance], &master_config, CLOCK_GetFreq(spi_clocks[obj->instance]));
    }
}

uint32_t spi_frequency(spi_t *obj, uint32_t hz)
{
    uint32_t busClock = CLOCK_GetFreq(spi_clocks[obj->instance]);
    uint32_t actual_br = DSPI_MasterSetBaudRate(spi_address[obj->instance], kDSPI_Ctar0, (uint32_t)hz, busClock);
    //Half clock period delay after SPI transfer
    DSPI_MasterSetDelayTimes(spi_address[obj->instance], kDSPI_Ctar0, kDSPI_LastSckToPcs, busClock, 500000000 / hz);
    return actual_br;
}

static int spi_write(spi_t *obj, uint32_t value)
{
    uint32_t rx_data;
    if (obj->slave) {
        DSPI_SlaveWriteDataBlocking(spi_address[obj->instance], value);
    } else {
        dspi_command_data_config_t command;
        DSPI_GetDefaultDataCommandConfig(&command);
        command.isEndOfQueue = true;

        DSPI_MasterWriteDataBlocking(spi_address[obj->instance], &command, (uint16_t)value);
        // trigger the send ?
        DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_TxFifoFillRequestFlag);
    }

    // wait rx buffer full
    while (!(DSPI_GetStatusFlags(spi_address[obj->instance]) & kDSPI_RxFifoDrainRequestFlag));
    rx_data = DSPI_ReadData(spi_address[obj->instance]);
    DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    return rx_data & 0xffff;
}

static uint32_t spi_symbol_size(spi_t *obj) {
    if (obj->bits > 16) { return 4; }
    else if (obj->bits > 8) { return 2; }
    return 1;
}

/// Take `len` symbols from `from`, reverse & shifts the bits before storing them in `to`.
/// `from` and `to` might be aliases to the same location.
///
/// @param obj   spi object.
/// @param from  source buffer.
/// @param to    destination buffer, must the same size or bigger than from
static void spi_reverse_bits(spi_t *obj, const void *from, void *to, uint32_t len) {
    uint32_t size = spi_symbol_size(obj);

    for (uint32_t i = 0; i < len; i++) {
        uint32_t val;
        switch(size) {
            case 1:
                val = ((uint8_t *)from)[i];
            break;
            case 2:
                val = ((uint16_t *)from)[i];
            break;
            case 4:
                val = ((uint32_t *)from)[i];
            break;
            default:
                // we could trap here
                break;

        }

        val = __RBIT(val) >> (32 - obj->bits);

        switch(size) {
            case 1:
                ((uint8_t *)to)[i] = val;
                break;
            case 2:
                ((uint16_t *)to)[i] = val;
                break;
            case 4:
                ((uint32_t *)to)[i] = val;
                break;
            default:
                break;
        }
    }
}

static void spi_irq_handler(spi_t *obj) {
    obj->done = true;
    if (obj->handler != NULL) {
        obj->transfered = obj->transfering;
        if (obj->slave && (obj->handle.slave.tx_length != 0)) {
            spi_reverse_bits(obj, obj->handle.slave.rx_buffer, obj->handle.slave.rx_buffer, obj->transfering);
            obj->handle.slave.rx_buffer += obj->transfering * spi_symbol_size(obj);

            obj->transfered += obj->transfering;
            obj->transfering = 0;

            spi_transfer_async(obj,
                    obj->handle.slave.tx_buffer, obj->handle.slave.tx_length,
                    obj->handle.slave.rx_buffer, obj->handle.slave.rx_length,
                    obj->handle.slave.fill_symbol, obj->handler, obj->ctx,
                    obj->handle.slave.hint);
        } else {
            spi_async_handler_f handler = obj->handler;
            void *ctx = obj->ctx;
            obj->handler = NULL;
            obj->ctx = NULL;

            spi_async_event_t event = {
                .transfered = obj->transfered,
                .error = false
            };

            handler(obj, ctx, &event);
        }
    }
}
static void spi_master_irq_handler(SPI_Type *base, dspi_master_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData);
}
static void spi_slave_irq_callback(SPI_Type *base, dspi_slave_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData);
}

// TODO:
// implement tx_len != rx_len
// implement cached send
// implement rbit in lsb first slave mode

uint32_t spi_transfer(spi_t *obj, const void *tx_buffer, uint32_t tx_length,
                      void *rx_buffer, uint32_t rx_length, const void *fill_symbol) {
    int total;

    if ((tx_length == 1) && (rx_length == 1)) {
#if 0
        if ((obj->slave) && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
            // cache & reverse in obj->buffer
            uint32_t cnt = MIN(tx_length, FSL_SPI_SLAVE_BUFFER_SZ);
            spi_reverse_bits(obj, tx_buffer, obj->handle.slave.buffer, cnt);
            obj->handle.slave.tx_buffer = ((uint8_t *)tx_buffer) + spi_symbol_size(obj)*cnt;
            obj->handle.slave.tx_length = tx_length - cnt;

            tx_buffer = obj->handle.slave.buffer;
            tx_length = cnt;
        }
#endif
        if (obj->bits <= 8) {
            *((uint8_t *)rx_buffer) = spi_write(obj, (tx_length == 1)?(((uint8_t*)tx_buffer)[0]):*(uint8_t *)fill_symbol);
        } else if (obj->bits <= 16) {
            *((uint16_t *)rx_buffer) = spi_write(obj, (tx_length == 1)?(((uint16_t*)tx_buffer)[0]):*(uint16_t *)fill_symbol);
        } else if (obj->bits <= 32) {
            *((uint32_t *)rx_buffer) = spi_write(obj, (tx_length == 1)?(((uint32_t*)tx_buffer)[0]):*(uint32_t *)fill_symbol);
        } // else trap !
        total = 1;

#if 0
        if ((obj->slave) && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
            // reverse in rx_buffer
            spi_reverse_bits(obj, rx_buffer, rx_buffer, rx_length);
        }
#endif
    } else {
        total = (tx_length > rx_length) ? tx_length : rx_length;

        obj->done = false;

        if (!spi_transfer_async(obj, tx_buffer, tx_length, rx_buffer, rx_length, fill_symbol, NULL, NULL, 0)) {
            return 0;
        }

        // wait for the end !
        while (!obj->done);

        DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    }

    return total;
}

bool spi_transfer_async(spi_t *obj, const void *tx, uint32_t tx_length, void *rx, uint32_t rx_length,
        const void *fill_symbol, spi_async_handler_f handler, void *ctx, DMAUsage hint)
{
    SPI_Type *spi = spi_address[obj->instance];

    obj->handler = handler;
    obj->ctx = ctx;

#if 0
    if ((obj->slave) && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
        // cache & reverse in obj->buffer
        uint32_t cnt = MIN(tx_length, FSL_SPI_SLAVE_BUFFER_SZ);
        spi_reverse_bits(obj, tx, obj->handle.slave.buffer, cnt);
        obj->handle.slave.tx_buffer = ((uint8_t *)tx) + spi_symbol_size(obj)*cnt;
        obj->handle.slave.tx_length = tx_length - cnt;
        obj->handle.slave.rx_buffer = rx;
        obj->handle.slave.rx_length = rx_length;
        obj->handle.slave.fill_symbol = fill_symbol;
        obj->handle.slave.hint = hint;

        tx = obj->handle.slave.buffer;
        tx_length = cnt;
    }
#endif
    obj->transfering = tx_length;
    DSPI_SetDummyData(spi_address[obj->instance], *(uint32_t *)fill_symbol);
    if (!obj->slave) {
        dspi_master_handle_t *handle = &obj->handle.master.handle;
        DSPI_MasterTransferCreateHandle(spi, handle, spi_master_irq_handler, obj);
        if (DSPI_MasterTransferNonBlocking(spi, handle, &(dspi_transfer_t){
            .txData = (uint8_t *)tx,
            .rxData = (uint8_t *)rx,
            .dataSize = obj->transfering,
            .configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcs0 | kDSPI_MasterPcsContinuous,
        }) != kStatus_Success) {
            return false;
        }
    } else {
        dspi_slave_handle_t *handle = &obj->handle.slave.handle;
        DSPI_SlaveTransferCreateHandle(spi, handle, spi_slave_irq_callback, obj);
        if (DSPI_SlaveTransferNonBlocking(spi, handle, &(dspi_transfer_t){
            .txData = (uint8_t *)tx,
            .rxData = (uint8_t *)rx,
            .dataSize = obj->transfering,
            .configFlags = kDSPI_SlaveCtar0,
        }) != kStatus_Success) {
            return false;
        }
    }
    return true;
}

void spi_transfer_async_abort(spi_t *obj) {
    SPI_Type *spi = spi_address[obj->instance];

    if (obj->slave) {
        DSPI_SlaveTransferAbort(spi, &obj->handle.slave.handle);
    } else {
        DSPI_MasterTransferAbort(spi, &obj->handle.master.handle);
    }
}

#endif
