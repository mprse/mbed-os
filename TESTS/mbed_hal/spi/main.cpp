/* mbed Microcontroller Library
 * Copyright (c) 2016 ARM Limited
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

#if !DEVICE_SPI
    #error [NOT_SUPPORTED] SPI not supported for this target
#endif

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "mbed.h"
#include "spi_api.h"

using namespace utest::v1;

// Based on K64F
#define SPI_MASTER_MOSI      PTD2
#define SPI_MASTER_MISO      PTD3
#define SPI_MASTER_SS        PTD0
#define SPI_MASTER_CLK       PTD1

#define SPI_SLAVE_MOSI       PTE1
#define SPI_SLAVE_MISO       PTE1
#define SPI_SLAVE_SS         PTE4
#define SPI_SLAVE_CLK        PTE2

#define SPI_MASTER_D1        SPI_MASTER_MOSI
#define SPI_MASTER_D2        SPI_MASTER_MISO
#define SPI_MASTER_D3        NC
#define SPI_MASTER_D4        NC

#define SPI_SLAVE_D1         SPI_SLAVE_MOSI
#define SPI_SLAVE_D2         SPI_SLAVE_MISO
#define SPI_SLAVE_D3         NC
#define SPI_SLAVE_D4         NC


static const spi_pins_s pins_simplex_master__master_transmitter = {SPI_MASTER_SS, NC, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_simplex_slave__master_transmitter = {SPI_SLAVE_SS, NC, SPI_SLAVE_MOSI, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_simplex_master__slave_transmitter = {SPI_MASTER_SS, SPI_MASTER_MISO, NC, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_simplex_slave__slave_transmitter = {SPI_SLAVE_SS, SPI_SLAVE_MISO, NC, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_half_duplex_master = {SPI_MASTER_SS, NC, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_half_duplex_slave = {SPI_SLAVE_SS, SPI_SLAVE_MISO, NC, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_full_duplex_master = {SPI_MASTER_SS, SPI_MASTER_MISO, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_full_duplex_slave = {SPI_SLAVE_SS, SPI_SLAVE_MISO, SPI_SLAVE_MOSI, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_dual_io_master = {SPI_MASTER_SS, SPI_MASTER_D1, SPI_MASTER_D2, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_dual_io_slave = {SPI_SLAVE_SS, SPI_SLAVE_D1, SPI_SLAVE_D2, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_quad_io_master = {SPI_MASTER_SS, SPI_MASTER_D1, SPI_MASTER_D2, SPI_MASTER_CLK, SPI_MASTER_D3, SPI_MASTER_D3};
static const spi_pins_s pins_quad_io_slave = {SPI_SLAVE_SS, SPI_SLAVE_D1, SPI_SLAVE_D2, SPI_SLAVE_CLK, SPI_SLAVE_D3, SPI_SLAVE_D4};

#define HIGH_SPI_FREQ   5000000   // 5 MHz
#define LOW_SPI_FREQ    400000    // 40 kHz

#define WORD_LEN_BITS_1     1
#define WORD_LEN_BITS_4     4
#define WORD_LEN_BITS_8     8
#define WORD_LEN_BITS_16   16
#define WORD_LEN_BITS_24   24
#define WORD_LEN_BITS_32   32


/* Test configuration.
 *
 * For SPI functional tests wire connection between SPI master and slave is required.
 * Select which connection is to be tested:
 * */
#define CONNECTION_SPI_MODE_SIMPLEX_MASTER_TRANSMITTER   false
#define CONNECTION_SPI_MODE_SIMPLEX_SLAVE_TRANSMITTER    false
#define CONNECTION_SPI_MODE_HALF_DUPLEX                  false
#define CONNECTION_SPI_MODE_FULL_DUPLEX                  false
#define CONNECTION_SPI_MODE_DUAL_IO                      false
#define CONNECTION_SPI_MODE_QUAD_IO                      false

/*
--- Pins connection for each mode. ---

1. SPI_MODE_SIMPLEX

Description:
    Unidirectional communication on a single wire.
    In this mode only slave can transfer data to the master or
    only master can transfer data to the slave.

   MASTER                      SLAVE
----------------------------------------
SPI_MASTER_CLK    ---->    SPI_SLAVE_CLK
SPI_MASTER_SS     ---->    SPI_SLAVE_SS

SPI_MASTER_MISO   <----    SPI_SLAVE_MISO  (slave transmitter)
                   or
SPI_MASTER_MOSI   ---->    SPI_SLAVE_MOSI  (master transmitter)


2. SPI_MODE_HALF_DUPLEX

Description:
    Bidirectional communication on a single wire.
    In this mode slave can send data to master and vice versa,
    but not in the same time.
    It is common to add a serial resistor on the half duplex data line to
    prevent possible temporary short-circuit connection, since master and
    slave nodes are not usually synchronised.

   MASTER                      SLAVE
----------------------------------------
SPI_MASTER_CLK    ---->    SPI_SLAVE_CLK
SPI_MASTER_SS     ---->    SPI_SLAVE_SS
SPI_MASTER_MOSI   <-R->    SPI_SLAVE_MISO

3. SPI_MODE_FULL_DUPLEX               // typical

Description:
    Bidirectional communication on two wires (MISO/MOSI).
    In this mode slave can send data to master and vice versa at the same time.

   MASTER                      SLAVE
----------------------------------------
SPI_MASTER_CLK    ---->    SPI_SLAVE_CLK
SPI_MASTER_SS     ---->    SPI_SLAVE_SS
SPI_MASTER_MISO   <----    SPI_SLAVE_MISO
SPI_MASTER_MOSI   ---->    SPI_SLAVE_MOSI

4. SPI_MODE_DUAL_IO

Description:
    Half-duplex communication on a two wires.
    In this mode slave can send data to master using two wires
    and vice versa, but not at the same time.

   MASTER                      SLAVE
----------------------------------------
SPI_MASTER_CLK    ---->    SPI_SLAVE_CLK
SPI_MASTER_SS     ---->    SPI_SLAVE_SS
SPI_MASTER_D0     <--->    SPI_SLAVE_D0
SPI_MASTER_D1     <--->    SPI_SLAVE_D1

5. SPI_MODE_QUAD_IO

Description:
    Half-duplex communication on a four wire.
    In this mode slave can send data to master using four wires
    and vice versa, but not at the same time.

   MASTER                      SLAVE
----------------------------------------
SPI_MASTER_CLK    ---->    SPI_SLAVE_CLK
SPI_MASTER_SS     ---->    SPI_SLAVE_SS
SPI_MASTER_D0     <--->    SPI_SLAVE_D0
SPI_MASTER_D1     <--->    SPI_SLAVE_D1
SPI_MASTER_D2     <--->    SPI_SLAVE_D2
SPI_MASTER_D3     <--->    SPI_SLAVE_D3

*/



typedef struct {
    spi_mode_t  mode;
    uint32_t    clock_frequency;
    uint32_t    word_length;
    bool        msb_first;
    bool        clock_phase;
    bool        clock_polarity;
    bool        continuous_mode;
    uint32_t    word_count;
} spi_comm_test_case_t;

/* Assume that the base transmission parameters are following:
 *
 * clock_frequency:   fast SPI clock frequency
 * word_length:       8 bits,
 * msb_first:         false,
 * clock_phase:       data are sampled on the leading (first) clock edge,
 * clock_polarity:    SPI clock non-inverted,
 * continuous_mode:   disabled.
 *
 */

static spi_comm_test_case_t spi_comm_test_cases[] = {
// base config
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , false    , false    , false    , 100 },
// test word count
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , false    , false    , false    , 0   },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , false    , false    , false    , 1   },
// test continuous mode
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , false    , false    , true     , 100 },
// test clock polarity and phase
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , false    , true     , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , true     , false    , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , false    , true     , true     , false    , 100 },
// test msb first
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_8    , true     , false    , false    , false    , 100 },
// test word length
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_1    , false    , false    , false    , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_4    , false    , false    , false    , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_16   , false    , false    , false    , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_24   , false    , false    , false    , false    , 100 },
    {HIGH_SPI_FREQ    , WORD_LEN_BITS_32   , false    , false    , false    , false    , 100 },
// test slow clock frequency
    {LOW_SPI_FREQ     , WORD_LEN_BITS_8    , false    , false    , false    , false    , 100 },
};

typedef struct {
    spi_mode_t mode;
    bool       connected;
    spi_pins_s master_pins;
    spi_pins_s slave_pins;

} spi_mode_connection_t;

#define CONNECTION_SPI_MODE_SIMPLEX_MASTER_TRANSMITTER   false
#define CONNECTION_SPI_MODE_SIMPLEX_SLAVE_TRANSMITTER    false
#define CONNECTION_SPI_MODE_HALF_DUPLEX                  false
#define CONNECTION_SPI_MODE_FULL_DUPLEX                  false
#define CONNECTION_SPI_MODE_DUAL_IO                      false
#define CONNECTION_SPI_MODE_QUAD_IO                      false

static const spi_pins_s pins_simplex_master__master_transmitter = {SPI_MASTER_SS, NC, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_simplex_slave__master_transmitter = {SPI_SLAVE_SS, NC, SPI_SLAVE_MOSI, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_simplex_master__slave_transmitter = {SPI_MASTER_SS, SPI_MASTER_MISO, NC, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_simplex_slave__slave_transmitter = {SPI_SLAVE_SS, SPI_SLAVE_MISO, NC, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_half_duplex_master = {SPI_MASTER_SS, NC, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_half_duplex_slave = {SPI_SLAVE_SS, SPI_SLAVE_MISO, NC, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_full_duplex_master = {SPI_MASTER_SS, SPI_MASTER_MISO, SPI_MASTER_MOSI, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_full_duplex_slave = {SPI_SLAVE_SS, SPI_SLAVE_MISO, SPI_SLAVE_MOSI, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_dual_io_master = {SPI_MASTER_SS, SPI_MASTER_D1, SPI_MASTER_D2, SPI_MASTER_CLK, NC, NC};
static const spi_pins_s pins_dual_io_slave = {SPI_SLAVE_SS, SPI_SLAVE_D1, SPI_SLAVE_D2, SPI_SLAVE_CLK, NC, NC};

static const spi_pins_s pins_quad_io_master = {SPI_MASTER_SS, SPI_MASTER_D1, SPI_MASTER_D2, SPI_MASTER_CLK, SPI_MASTER_D3, SPI_MASTER_D3};
static const spi_pins_s pins_quad_io_slave = {SPI_SLAVE_SS, SPI_SLAVE_D1, SPI_SLAVE_D2, SPI_SLAVE_CLK, SPI_SLAVE_D3, SPI_SLAVE_D4};


static spi_mode_connection_t comm_test_modes[] = {
    {SPI_MODE_SIMPLEX_MASTER_TRANSMITTER, CONNECTION_SPI_MODE_SIMPLEX_MASTER_TRANSMITTER, pins_simplex_master__master_transmitter, pins_simplex_slave__master_transmitter},
    {SPI_MODE_SIMPLEX_SLAVE_TRANSMITTER,  CONNECTION_SPI_MODE_SIMPLEX_SLAVE_TRANSMITTER,  pins_simplex_master__slave_transmitter,  pins_simplex_slave__slave_transmitter}
    {SPI_MODE_HALF_DUPLEX,                CONNECTION_SPI_MODE_HALF_DUPLEX,                pins_half_duplex_master,                 pins_half_duplex_slave},
    {SPI_MODE_FULL_DUPLEX,                CONNECTION_SPI_MODE_FULL_DUPLEX,                pins_full_duplex_master,                 pins_full_duplex_slave},
    {SPI_MODE_DUAL_IO,                    CONNECTION_SPI_MODE_DUAL_IO,                    pins_dual_io_master,                     pins_dual_io_slave},
    {SPI_MODE_QUAD_IO,                    CONNECTION_SPI_MODE_QUAD_IO,                    pins_quad_io_master,                     pins_quad_io_slave}
};


static bool validate_config(uint32_t mode_idx, uint32_t tc_idx)
{


}

void spi_communication_functional_test()
{
    for (uint32_t mode_idx = 0; mode_idx < (sizeof(comm_test_modes)/sizeof(spi_mode_connection_t)); mode_idx++) {

        /* Skip if the wire connection for the tested mode is not provided. */
        if (!comm_test_modes[mode_idx].connected) {
            continue;
        }

        for (uint32_t tc_idx = 0; tc_idx < (sizeof(spi_comm_test_cases)/sizeof(spi_comm_test_case_t)); tc_idx++) {






        }
    }



    TEST_ASSERT_TRUE(complete);
}


Case cases[] = {
    Case("SPI test", spi_test),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(20, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases);

int main()
{
    Harness::run(specification);
}
