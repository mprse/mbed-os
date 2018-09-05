/*
 * Copyright (c) 2018 ARM Limited
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
    #error [NOT_SUPPORTED] SPI is not supported on this platform, add 'DEVICE_SPI' definition to your platform.
#endif

#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "spi_api.h"
#include "pinmap.h"

using namespace utest::v1;

/* Test configuration.
 *
 * This test can be run only if wire connection and pin configuration is provided.
 * Two SPI channels are required. One will operate as master other one as slave.
 * The following wire connection need to be provided to run the test:
 *
 * SPI_MASTER_MOSI  -------------------  SPI_SLAVE_MOSI
 * SPI_MASTER_MISO  -------------------  SPI_SLAVE_MISO
 * SPI_MASTER_SS    -------------------  SPI_SLAVE_SS
 * SPI_MASTER_CLK   -------------------  SPI_SLAVE_CLK
 *
 * */
#define SPI_MASTER_NAME      SPI_1
#define SPI_MASTER_MOSI      PA_7
#define SPI_MASTER_MISO      PA_6
#define SPI_MASTER_SS        PA_4
#define SPI_MASTER_CLK       PA_5

#define SPI_SLAVE_NAME       SPI_3
#define SPI_SLAVE_MOSI       PE_6
#define SPI_SLAVE_MISO       PE_5
#define SPI_SLAVE_SS         PE_4
#define SPI_SLAVE_CLK        PE_2

/* Transmission configuration. */
#define NUMBER_OF_MESSAGES   3

static spi_t spi_master = { 0 };
static spi_t spi_slave = { 0 };

#define BUF_SIZE 5

static uint8_t m_tx_buff_uint8[BUF_SIZE];
static uint8_t m_rx_buff_uint8[BUF_SIZE];
static uint8_t s_tx_buff_uint8[BUF_SIZE];
static uint8_t s_rx_buff_uint8[BUF_SIZE];
static uint16_t m_tx_buff_uint16[BUF_SIZE];
static uint16_t m_rx_buff_uint16[BUF_SIZE];
static uint16_t s_tx_buff_uint16[BUF_SIZE];
static uint16_t s_rx_buff_uint16[BUF_SIZE];
static uint32_t m_tx_buff_uint32[BUF_SIZE];
static uint32_t m_rx_buff_uint32[BUF_SIZE];
static uint32_t s_tx_buff_uint32[BUF_SIZE];
static uint32_t s_rx_buff_uint32[BUF_SIZE];

static void *p_m_tx_buf;
static void *p_m_rx_buf;
static void *p_s_tx_buf;
static void *p_s_rx_buf;
static void *p_fill_sym;

static Semaphore join(0, 2);

#define CAPABILITY_WORD_LENGTH_8 (1<<7)

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)

static const uint8_t fill_symbol_8 = 0x55;
static const uint16_t fill_symbol_16 = 0x5555;
static const uint32_t fill_symbol_32 = 0x55555555;

/* There is no tolerance defined how much requested frequency can differ from the actual frequency.
 * The driver gives as fast as it can without exceeding the requested speed.
 * Since minimum frequency divisor is 2 and we will verify only values
 * within required frequency range lets assume actual frequency must be
 * greater than or equal to (requested speed / 2).
 */
#define FREQ_TOLERANCE(requested_freq) (requested_freq / 2)

/* SPI format/frequency configuration. */
typedef struct {
    uint8_t symbol_size;
    spi_mode_t mode;
    spi_bit_ordering_t bit_ordering;
    uint32_t freq_hz;
} config_test_case_t;

/* Array witch test cases which represents different SPI configs for testing. */
static config_test_case_t test_cases[] = {
        /* default config */
        {8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* symbol size testing */
        {0, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {1, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {7, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {9, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {15, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {16, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {17, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {31, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {32, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* mode testing */
        {8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        {8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* bit ordering testing */
        {8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_1MHZ},
        /* freq testing */
        {8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_200KHZ},
        {8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_2MHZ},
};

static void clear_buf(void * addr, uint32_t size)
{
    char *p_char = (char*)addr;

    for (int i = 0; i < size; i++) {
        p_char[i] = 0;
    }
}

static void transfer_slave() {
    int32_t count = spi_transfer(&spi_slave, p_s_tx_buf, BUF_SIZE, p_s_rx_buf, BUF_SIZE, p_fill_sym);
    join.release();
}

static void transfer_master() {
    int32_t count = spi_transfer(&spi_master, p_m_tx_buf, BUF_SIZE, p_m_rx_buf, BUF_SIZE, p_fill_sym);
    join.release();
}


/* Test that spi_transfer() can successfully perform transfer in master mode
 *  (TX/RX buffers are defined and have the same sizes) and returns the number of
 *  symbols clocked on the bus during this transfer. */
void test_transfer_master()
{
    spi_capabilities_t capabilities =
    { 0 };

    spi_get_capabilities(spi_get_module(SPI_MASTER_MOSI,
                                        SPI_MASTER_MISO,
                                        SPI_MASTER_CLK),
                         SPI_MASTER_SS,
                         &capabilities);

    spi_init(&spi_master, false, SPI_MASTER_MOSI, SPI_MASTER_MISO, SPI_MASTER_CLK, SPI_MASTER_SS);
    spi_init(&spi_slave, true, SPI_SLAVE_MOSI, SPI_SLAVE_MISO, SPI_SLAVE_CLK, SPI_SLAVE_SS);

    for (uint32_t tc_id = 0; tc_id < (sizeof(test_cases) / sizeof(config_test_case_t)); tc_id++) {

        if (capabilities.word_length & (1 << (test_cases[tc_id].symbol_size - 1))) {

            spi_format(&spi_master, test_cases[tc_id].symbol_size, test_cases[tc_id].mode,
                    test_cases[tc_id].bit_ordering);
            spi_frequency(&spi_master, test_cases[tc_id].freq_hz);

            spi_format(&spi_slave, test_cases[tc_id].symbol_size, test_cases[tc_id].mode,
                    test_cases[tc_id].bit_ordering);
            spi_frequency(&spi_slave, test_cases[tc_id].freq_hz);

            if (test_cases[tc_id].symbol_size <= 8) {
                p_m_tx_buf = m_tx_buff_uint8;
                p_m_rx_buf = m_rx_buff_uint8;
                p_s_tx_buf = s_tx_buff_uint8;
                p_s_rx_buf = s_rx_buff_uint8;

                clear_buf(p_m_tx_buf, sizeof(m_tx_buff_uint8));
                clear_buf(p_m_rx_buf, sizeof(m_tx_buff_uint8));
                clear_buf(p_s_tx_buf, sizeof(m_tx_buff_uint8));
                clear_buf(p_s_rx_buf, sizeof(m_tx_buff_uint8));

                m_tx_buff_uint8[0] = 'a';
                m_tx_buff_uint8[1] = 'b';
                m_tx_buff_uint8[2] = 'c';
                m_tx_buff_uint8[3] = 'd';
                m_tx_buff_uint8[4] = 0;
                s_tx_buff_uint8[0] = 'w';
                s_tx_buff_uint8[1] = 'x';
                s_tx_buff_uint8[2] = 'y';
                s_tx_buff_uint8[3] = 'z';
                s_tx_buff_uint8[4] = 0;

                p_fill_sym = (void*) &fill_symbol_8;
            } else if (test_cases[tc_id].symbol_size <= 16) {
                p_m_tx_buf = m_tx_buff_uint16;
                p_m_rx_buf = m_rx_buff_uint16;
                p_s_tx_buf = s_tx_buff_uint16;
                p_s_rx_buf = s_rx_buff_uint16;
                p_fill_sym = (void*) &fill_symbol_16;
            } else {
                p_m_tx_buf = m_tx_buff_uint32;
                p_m_rx_buf = m_rx_buff_uint32;
                p_s_tx_buf = s_tx_buff_uint32;
                p_s_rx_buf = s_rx_buff_uint32;
            }

            Thread tslave;
            tslave.start(callback(transfer_slave));

            Thread tmaster;
            tmaster.start(callback(transfer_master));

            // wait on semphr
            uint32_t i = 0;
            uint64_t cnt = 0;
            while (i < 2 && cnt<100) {
                i += join.wait(0);
                cnt += 1;
            }

            printf("master send: %s \n", p_m_tx_buf);
            printf("master received: %s \n", p_m_rx_buf);

            printf("slave send: %s \n", p_s_tx_buf);
            printf("slave received: %s \n", p_s_rx_buf);



            break;
        }
    }

    spi_free(&spi_master);
    spi_free(&spi_slave);
}


utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(40, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

utest::v1::status_t greentea_failure_handler(const Case * const source, const failure_t reason)
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

utest::v1::status_t greentea_failure_handler_abort(const Case * const source, const failure_t reason)
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_ABORT;
}

Case cases[] = {
    Case("SPI - master transfer test", test_transfer_master, greentea_failure_handler)
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
