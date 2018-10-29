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

void spi_get_capabilities(SPIName name, PinName ssel, spi_capabilities_t *cap)
{
    cap->word_length = 0x00008080;
    cap->support_slave_mode = true;
    cap->half_duplex = true;

    cap->minimum_frequency = 200000;
    cap->maximum_frequency = 4000000;
}

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
    MBED_ASSERT(spi_module != NC);

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
    obj->is_slave = is_slave;
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

    if (obj->is_slave) {
        /* Slave config */
        DSPI_SlaveGetDefaultConfig(&slave_config);
        slave_config.whichCtar = kDSPI_Ctar0;
        slave_config.ctarConfig.bitsPerFrame = (uint32_t)bits;
        slave_config.ctarConfig.cpol = cpol;
        slave_config.ctarConfig.cpha = cpha;

        DSPI_SlaveInit(spi_address[obj->instance], &slave_config);
    } else {
        /* Master config */
        DSPI_MasterGetDefaultConfig(&master_config);
        master_config.ctarConfig.bitsPerFrame = (uint32_t)bits;
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
    DSPI_MasterSetDelayTimes(spi_address[obj->instance], kDSPI_Ctar0, kDSPI_LastSckToPcs, busClock, 2 * (1000000000 / hz));
    DSPI_MasterSetDelayTimes(spi_address[obj->instance], kDSPI_Ctar0, kDSPI_PcsToSck, busClock, 2 * (1000000000 / hz));
    DSPI_MasterSetDelayTimes(spi_address[obj->instance], kDSPI_Ctar0, kDSPI_BetweenTransfer, busClock, 0);
    return actual_br;
}

static int spi_write(spi_t *obj, uint32_t value)
{
    uint32_t rx_data;
    if (obj->is_slave) {
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

static void spi_irq_handler(spi_t *obj, status_t status) {
    if (obj->handler != NULL) {
        spi_async_handler_f handler = obj->handler;
        void *ctx = obj->ctx;
        obj->handler = NULL;
        obj->ctx = NULL;

        spi_async_event_t event = {
            .transfered = obj->transfer_len,
            .error = false
        };

        handler(obj, ctx, &event);
    }
}
static void spi_master_irq_handler(SPI_Type *base, dspi_master_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData, status);
}
static void spi_slave_irq_callback(SPI_Type *base, dspi_slave_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData, status);
}

static void spi_sync_transfer_handler(spi_t *obj, void *ctx, spi_async_event_t *event) {
    obj->transfered = event->transfered;
}

static uint32_t spi_symbol_size(spi_t *obj) {
    if (obj->bits > 16) { return 4; }
    else if (obj->bits > 8) { return 2; }
    return 1;
}

static uint32_t spi_get_symbol(spi_t *obj, const void *from, uint32_t i) {
    uint32_t val = 0;
    switch (spi_symbol_size(obj)) {
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
            // TODO: TRAP ?
            break;
    }
    return val;
}

static void spi_set_symbol(spi_t *obj, void *to, uint32_t i, uint32_t val) {
    switch (spi_symbol_size(obj)) {
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
            // TODO: TRAP ?
            break;
    }
}

uint32_t spi_transfer(spi_t *obj, const void *tx_buffer, uint32_t tx_length,
                      void *rx_buffer, uint32_t rx_length, const void *fill) {
    uint32_t total = 0;
    if ((tx_length <= 1) && (rx_length <= 1)) {
        uint32_t val_o = 0;
        if (tx_length != 0) {
            val_o = spi_get_symbol(obj, tx_buffer, 0);
        } else {
            val_o = spi_get_symbol(obj, fill, 0);
        }
        uint32_t val_i = spi_write(obj, val_o);

        if (rx_length != 0) {
            spi_set_symbol(obj, rx_buffer, 0, val_i);
        }
        total = 1;
    } else {
        SPI_Type *spi = spi_address[obj->instance];
        total = (tx_length > rx_length) ? tx_length : rx_length;

        // Default write is done in each and every call, in future can create HAL API instead
        DSPI_SetDummyData(spi, *(uint32_t *)fill);

        DSPI_TransferBlockingWithLimit(spi, &(dspi_transfer_t){
              .txData = (uint8_t *)tx_buffer,
              .rxData = (uint8_t *)rx_buffer,
              .dataSize = total * spi_symbol_size(obj),
              .configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcs0 | kDSPI_MasterPcsContinuous,
        }, tx_length, rx_length);

        DSPI_ClearStatusFlags(spi, kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    }

    return total;
}

bool spi_transfer_async(spi_t *obj, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len,
        const void *fill, spi_async_handler_f handler, void *ctx, DMAUsage hint)
{
    SPI_Type *spi = spi_address[obj->instance];

    obj->handler = handler;
    obj->ctx = ctx;

    DSPI_SetDummyData(spi, *(uint32_t *)fill);
    uint32_t len = (rx_len>tx_len)?rx_len:tx_len;
    obj->transfer_len = len;

    if (!obj->is_slave) {
        dspi_master_handle_t *handle = &obj->u.master.handle;
        DSPI_MasterTransferCreateHandle(spi, handle, spi_master_irq_handler, obj);
        if (DSPI_MasterTransferNonBlocking(spi, handle, &(dspi_transfer_t){
            .txData = (uint8_t *)tx,
            .rxData = (uint8_t *)rx,
            .dataSize = len * spi_symbol_size(obj),
            .configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcs0 | kDSPI_MasterPcsContinuous,
        }) != kStatus_Success) {
            return false;
        }
        DSPI_ClearStatusFlags(spi, kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    } else {
        dspi_slave_handle_t *handle = &obj->u.slave.handle;
        DSPI_SlaveTransferCreateHandle(spi, handle, spi_slave_irq_callback, obj);
        if (DSPI_SlaveTransferNonBlocking(spi, handle, &(dspi_transfer_t){
            .txData = (uint8_t *)tx,
            .rxData = (uint8_t *)rx,
            .dataSize = len * spi_symbol_size(obj),
            .configFlags = kDSPI_SlaveCtar0,
        }) != kStatus_Success) {
            return false;
        }
    }
    return true;
}

void spi_transfer_async_abort(spi_t *obj) {
    SPI_Type *spi = spi_address[obj->instance];

    if (obj->is_slave) {
        DSPI_SlaveTransferAbort(spi, &obj->u.slave.handle);
    } else {
        DSPI_MasterTransferAbort(spi, &obj->u.master.handle);
    }
}

#endif
