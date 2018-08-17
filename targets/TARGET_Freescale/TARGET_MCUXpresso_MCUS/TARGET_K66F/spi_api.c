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

/* Array of SPI peripheral base address. */
static SPI_Type *const spi_address[] = SPI_BASE_PTRS;
/* Array of SPI bus clock frequencies */
static clock_name_t const spi_clocks[] = SPI_CLOCK_FREQS;

SPIName spi_get_module(PinName mosi, PinName miso, PinName sclk) {
    uint32_t spi_mosi = pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    uint32_t spi_miso = pinmap_peripheral(miso, PinMap_SPI_MISO);
    uint32_t spi_sclk = pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    
    uint32_t spi_data = pinmap_merge(spi_mosi, spi_miso);
    return pinmap_merge(spi_data, spi_sclk);
}

void spi_init(spi_t *obj, bool is_slave, PinName mosi, PinName miso, PinName sclk, PinName ssel)
{
    // determine the SPI to use
    uint32_t spi_module = (uint32_t)spi_get_module(mosi, miso, sclk);
    uint32_t spi_ssel = pinmap_peripheral(ssel, PinMap_SPI_SSEL);
    MBED_ASSERT(spi_module != NC);

    obj->instance = pinmap_merge(spi_module, spi_ssel);
    MBED_ASSERT((int)obj->instance != NC);

    // pin out the spi pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
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

static inline int spi_readable(spi_t * obj)
{
    return (DSPI_GetStatusFlags(spi_address[obj->instance]) & kDSPI_RxFifoDrainRequestFlag);
}

static int spi_master_write(spi_t *obj, int value)
{
    dspi_command_data_config_t command;
    uint32_t rx_data;
    DSPI_GetDefaultDataCommandConfig(&command);
    command.isEndOfQueue = true;

    DSPI_MasterWriteDataBlocking(spi_address[obj->instance], &command, (uint16_t)value);

    DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_TxFifoFillRequestFlag);

    // wait rx buffer full
    while (!spi_readable(obj));
    rx_data = DSPI_ReadData(spi_address[obj->instance]);
    DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    return rx_data & 0xffff;
}

uint32_t spi_transfer(spi_t *obj, const void *tx_buffer, uint32_t tx_length,
                      void *rx_buffer, uint32_t rx_length, const void *write_fill) {
    int total;
    if (tx_length == 1 && rx_length == 1) {
        if (obj->bits <= 8) {
            ((uint8_t *)rx_buffer)[0] = spi_master_write(obj, (tx_length == 1)?(((uint8_t*)tx_buffer)[0]):*(uint8_t *)write_fill);
        } else if (obj->bits <= 16) {
            ((uint16_t *)rx_buffer)[0] = spi_master_write(obj, (tx_length == 1)?(((uint16_t*)tx_buffer)[0]):*(uint16_t *)write_fill);
        } else if (obj->bits <= 32) {
            ((uint32_t *)rx_buffer)[0] = spi_master_write(obj, (tx_length == 1)?(((uint32_t*)tx_buffer)[0]):*(uint32_t *)write_fill);
        } // else trap !
        total = 1;
    } else {
        total = (tx_length > rx_length) ? tx_length : rx_length;

        // Default write is done in each and every call, in future can create HAL API instead
        DSPI_SetDummyData(spi_address[obj->instance], *(uint32_t *)write_fill);

        DSPI_MasterTransferBlocking(spi_address[obj->instance], &(dspi_transfer_t){
              .txData = (uint8_t *)tx_buffer,
              .rxData = (uint8_t *)rx_buffer,
              .dataSize = total,
              .configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcs0 | kDSPI_MasterPcsContinuous,
        });

        DSPI_ClearStatusFlags(spi_address[obj->instance], kDSPI_RxFifoDrainRequestFlag | kDSPI_EndOfQueueFlag);
    }

    return total;
}

#endif
