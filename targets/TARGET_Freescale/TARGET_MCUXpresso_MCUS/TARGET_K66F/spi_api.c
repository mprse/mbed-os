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
    cap->word_length = (1<<7 | 1<<15);
    cap->support_slave_mode = true;
    cap->half_duplex = true;

    cap->minimum_frequency = 200000;
    cap->maximum_frequency = 2000000;
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
    obj->u.slave.reverse = NULL;
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
        slave_config.ctarConfig.bitsPerFrame = (uint32_t)bits;;
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

static void spi_reverse_bits_at(spi_t *obj, const void *from, void *to, uint32_t i) {
    uint32_t val;
    val = spi_get_symbol(obj, from, i);

    val = __RBIT(val) >> (32 - obj->bits);

    spi_set_symbol(obj, to, i, val);
}

/// Take `len` symbols from `from`, reverse & shifts the bits before storing them in `to`.
/// `from` and `to` might be aliases to the same location.
///
/// @param obj   spi object.
/// @param from  source buffer.
/// @param to    destination buffer, must the same size or bigger than from
static void spi_reverse_bits_in_place(spi_t *obj, void *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        spi_reverse_bits_at(obj, buf, buf, i);
    }
}

//void my_log(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

static void spi_irq_handler(spi_t *obj) {
    //my_log(0, obj->is_slave, DSPI_GetStatusFlags(spi_address[obj->instance]), ((SPI_Type *)spi_address[obj->instance])->RSER, 0);
    if (obj->handler != NULL) {
        obj->transfered = obj->transfering;

        if ((obj->pending.rx != NULL) || (obj->pending.tx != NULL)) {
            //my_log(0, obj->is_slave, (uint32_t)obj->pending.rx, (uint32_t)obj->pending.tx, 2);
            //my_log(0, obj->is_slave, obj->pending.rx_len, obj->pending.tx_len, 3);
            spi_transfer_async(obj,
                    obj->pending.tx, obj->pending.tx_len,
                    obj->pending.rx, obj->pending.rx_len,
                    obj->pending.fill, obj->handler, obj->ctx, obj->pending.hint);
        } else {
            spi_async_handler_f handler = obj->handler;
            void *ctx = obj->ctx;
            obj->handler = NULL;
            obj->ctx = NULL;

            //my_log(0, obj->is_slave, (uint32_t)obj->pending.rx, (uint32_t)obj->pending.tx, 4);
            //my_log(0, obj->is_slave, (uint32_t)obj->u.slave.reverse, 0, 5);

            if (obj->is_slave && obj->u.slave.reverse != NULL) {
                spi_reverse_bits_in_place(obj, obj->u.slave.reverse, obj->transfered);
                obj->u.slave.reverse = NULL;
            }
            
            spi_async_event_t event = {
                .transfered = obj->transfered,
                .error = false
            };

            handler(obj, ctx, &event);
        }
    }
    //my_log(0, obj->is_slave, DSPI_GetStatusFlags(spi_address[obj->instance]), ((SPI_Type *)spi_address[obj->instance])->RSER, 6);
}
static void spi_master_irq_handler(SPI_Type *base, dspi_master_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData);
}
static void spi_slave_irq_callback(SPI_Type *base, dspi_slave_handle_t *handle, status_t status, void *userData) {
    spi_irq_handler(userData);
}

static void spi_sync_transfer_handler(spi_t *obj, void *ctx, spi_async_event_t *event) {
    obj->done = true;
}

uint32_t spi_transfer(spi_t *obj, const void *tx_buffer, uint32_t tx_length,
                      void *rx_buffer, uint32_t rx_length, const void *fill_symbol) {
    int total;

    //my_log(2, obj->is_slave, (uint32_t)rx_buffer, (uint32_t)tx_buffer, tx_length);
    if ((tx_length == 1) || (rx_length == 1)) {
        uint32_t val_i = 0;
        uint32_t val_o = 0;

        if (tx_length != 0) {
            val_o = spi_get_symbol(obj, tx_buffer, 0);
            if (obj->is_slave && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
                spi_reverse_bits_at(obj, &val_o, &val_o, 0);
            }
        }

        if (obj->half_duplex) {
            uint32_t fill = spi_get_symbol(obj, fill_symbol, 0);

            if (obj->is_slave && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
                spi_reverse_bits_at(obj, &fill, &fill, 0);
            }

            if ((tx_length != 0) && !obj->is_slave) {
                spi_write(obj, val_o);
            }
            val_i = spi_write(obj, fill);
            if ((tx_length != 0) && obj->is_slave) {
                spi_write(obj, val_o);
            }
        } else {
            if (tx_buffer == 0) {
                val_o = spi_get_symbol(obj, fill_symbol, 0);
                if (obj->is_slave && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
                    spi_reverse_bits_at(obj, &val_o, &val_o, 0);
                }
            }

            val_i = spi_write(obj, val_o);
        }

        if (rx_length == 1) {
            if ((obj->is_slave) && (obj->order == SPI_BIT_ORDERING_LSB_FIRST)) {
                spi_reverse_bits_at(obj, &val_i, &val_i, 0);
            }
            spi_set_symbol(obj, rx_buffer, 0, val_i);
        }
        total = 1;
    } else {
        total = (tx_length > rx_length) ? tx_length : rx_length;

        obj->done = false;

        if (!spi_transfer_async(obj, tx_buffer, tx_length, rx_buffer, rx_length, fill_symbol, spi_sync_transfer_handler, NULL, 0)) {
            return 0;
        }

        // wait for the end !
        while (!obj->done);
    }

    return total;
}

static void spi_pending_transfer_set(spi_t *obj, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len, const void *fill, DMAUsage hint) {
    obj->pending.tx = tx;
    obj->pending.tx_len = tx_len;
    obj->pending.tx = NULL;
    obj->pending.tx_len = 0;
    obj->pending.rx = rx;
    obj->pending.rx_len = rx_len;
    obj->pending.fill = fill;
    obj->pending.hint = hint;
}

bool spi_transfer_async(spi_t *obj, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len,
        const void *fill, spi_async_handler_f handler, void *ctx, DMAUsage hint)
{
    SPI_Type *spi = spi_address[obj->instance];

    obj->handler = handler;
    obj->ctx = ctx;
    //my_log(1, obj->is_slave, tx_len, rx_len, 0);

    if (obj->half_duplex) {
        if (!obj->is_slave && (tx_len != 0)) {
            spi_pending_transfer_set(obj, NULL, 0, rx, rx_len, fill, hint);
            rx = NULL;
            rx_len = 0;
            obj->transfering = tx_len;
        } else if (rx_len != 0) {
            spi_pending_transfer_set(obj, tx, tx_len, NULL, 0, fill, hint);
            tx = NULL;
            tx_len = 0;
            obj->transfering = rx_len;
        }
    } else if (tx_len > rx_len) {
        const uint8_t *next_tx = ((const uint8_t *)tx) + rx_len * spi_symbol_size(obj);
        spi_pending_transfer_set(obj, next_tx, tx_len - rx_len, NULL, 0, fill, hint);
        obj->transfering = rx_len;
    } else if (tx_len < rx_len) {
        uint8_t *next_rx = ((uint8_t *)rx) + tx_len * spi_symbol_size(obj);
        spi_pending_transfer_set(obj, NULL, 0, next_rx, rx_len - tx_len, fill, hint);
        obj->transfering = tx_len;
    } else {
        obj->transfering = tx_len;
    }
    //my_log(1, obj->is_slave, rx, tx, 1);

    if (obj->is_slave && (obj->order == SPI_BIT_ORDERING_LSB_FIRST) && (tx != NULL)) {
        obj->transfering = MIN(FSL_SPI_SLAVE_BUFFER_SZ / spi_symbol_size(obj), obj->transfering);

        if (obj->u.slave.reverse == NULL) {
            obj->u.slave.reverse = rx;
            //my_log(1, obj->is_slave, (uint32_t)obj->u.slave.reverse, 0xDEADBEEF, 2);
        }

        if (obj->half_duplex) {
            tx = ((const uint8_t *)tx) + obj->transfering * spi_symbol_size(obj);
            tx_len -= obj->transfering;
            spi_pending_transfer_set(obj, tx, tx_len, obj->pending.rx, obj->pending.rx_len, fill, hint);
        } else {
            const uint8_t *next_tx = ((const uint8_t *)tx) + obj->transfering * spi_symbol_size(obj);
            uint8_t *next_rx = NULL;
            uint32_t next_rx_len = rx_len;
            if (rx != NULL) {
                next_rx_len -= obj->transfering;
                if (next_rx_len == 0) {
                    next_rx = NULL;
                } else {
                    next_rx = ((uint8_t *)rx) + obj->transfering * spi_symbol_size(obj);
                }
            }
            spi_pending_transfer_set(obj, next_tx, tx_len - obj->transfering, next_rx, next_rx_len, fill, hint);
        }
        // load from cache
        for (uint32_t i = 0; i < obj->transfering; i++) {
            spi_reverse_bits_at(obj, tx, obj->u.slave.buffer, i);
        }

        tx = obj->u.slave.buffer;
    }

    //my_log(1, obj->is_slave, obj->transfering, obj->transfered, 3);

    DSPI_SetDummyData(spi_address[obj->instance], *(uint32_t *)fill);
    if (!obj->is_slave) {
        dspi_master_handle_t *handle = &obj->u.master.handle;
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
        dspi_slave_handle_t *handle = &obj->u.slave.handle;
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
    //my_log(1, obj->is_slave, DSPI_GetStatusFlags(spi_address[obj->instance]), 0, 4);
    return true;
}

void spi_transfer_async_abort(spi_t *obj) {
    SPI_Type *spi = spi_address[obj->instance];

    if (obj->is_slave) {
        DSPI_SlaveTransferAbort(spi, &obj->u.slave.handle);
        obj->u.slave.reverse = NULL;
    } else {
        DSPI_MasterTransferAbort(spi, &obj->u.master.handle);
    }
}

#endif
