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

DigitalOut led1(LED1);



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

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)

static spi_t spi_slave = { 0 };

#define DEFAULT_CFG 0

#define TEST_SYM_CNT 5

typedef enum {
    FULL_DUPLEX,
    HALF_DUPLEX_MOSI,
    HALF_DUPLEX_MISO
} duplex_t;

/* SPI format/frequency configuration. */
typedef struct {
    uint8_t symbol_size;
    spi_mode_t mode;
    spi_bit_ordering_t bit_ordering;
    uint32_t freq_hz;
    uint32_t master_tx_cnt;
    uint32_t master_rx_cnt;
    uint32_t slave_tx_cnt;
    uint32_t slave_rx_cnt;
    bool master_tx_defined;
    bool master_rx_defined;
    bool slave_tx_defined;
    bool slave_rx_defined;
    bool auto_ss;
    duplex_t full_duplex;
} config_test_case_t;

#define CONFIG_LEN (sizeof(config_test_case_t))
#define CONFIG_STATUS_LEN 1

static uint8_t tx_pattern_8[TEST_SYM_CNT] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint16_t tx_pattern_16[TEST_SYM_CNT] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
static uint32_t tx_pattern_32[TEST_SYM_CNT] = {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555};

static uint8_t rx_pattern_8[TEST_SYM_CNT] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static uint16_t rx_pattern_16[TEST_SYM_CNT] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
static uint32_t rx_pattern_32[TEST_SYM_CNT] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};

static uint8_t s_tx_buff_uint8[TEST_SYM_CNT];
static uint8_t s_rx_buff_uint8[TEST_SYM_CNT];
static uint16_t s_tx_buff_uint16[TEST_SYM_CNT];
static uint16_t s_rx_buff_uint16[TEST_SYM_CNT];
static uint32_t s_tx_buff_uint32[TEST_SYM_CNT];
static uint32_t s_rx_buff_uint32[TEST_SYM_CNT];

static const uint8_t fill_symbol_8 = (uint8_t)0xFF;
static const uint8_t fill_symbol_16 = (uint8_t)0xFFFF;
static const uint8_t fill_symbol_32 = (uint8_t)0xFFFFFFFF;

static void *p_tx_buf;
static void *p_rx_buf;
static void *p_fill_sym;

static void clear_buf(void * addr, uint32_t size)
{
    char *p_char = (char*)addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = 0;
    }
}

static void init_buffers(config_test_case_t *config)
{
    uint32_t symbol_size = config->symbol_size;

    if (symbol_size <= 8) {
        p_tx_buf = s_tx_buff_uint8;
        p_rx_buf = s_rx_buff_uint8;
        p_fill_sym = (void*) &fill_symbol_8;
        memcpy(s_tx_buff_uint8, tx_pattern_8, sizeof(s_tx_buff_uint8));
        clear_buf(s_rx_buff_uint8, sizeof(s_rx_buff_uint8));
    } else if (symbol_size <= 16) {
        p_tx_buf = s_tx_buff_uint16;
        p_rx_buf = s_rx_buff_uint16;
        p_fill_sym = (void*) &fill_symbol_16;
        memcpy(s_tx_buff_uint16, tx_pattern_16, sizeof(s_tx_buff_uint16));
        clear_buf(s_rx_buff_uint16, sizeof(s_rx_buff_uint16));
    } else {
        p_tx_buf = s_tx_buff_uint32;
        p_rx_buf = s_rx_buff_uint32;
        p_fill_sym = (void*) &fill_symbol_32;
        memcpy(s_tx_buff_uint32, tx_pattern_32, sizeof(s_tx_buff_uint32));
        clear_buf(s_rx_buff_uint32, sizeof(s_rx_buff_uint32));
    }


    if (!config->slave_tx_defined) {
        p_tx_buf = NULL;
    }

    if (!config->slave_rx_defined) {
        p_rx_buf = NULL;
    }
}

void ticker_event_handler_stub(const ticker_data_t *const ticker)
{
    printf("----> Ticker interrupt!!! \r\n");
}

int main()
{
    int32_t count;
    int8_t status;
    spi_capabilities_t capabilities =
        { 0 };

    osKernelSuspend();
    set_lp_ticker_irq_handler(ticker_event_handler_stub);
    set_us_ticker_irq_handler(ticker_event_handler_stub);

    printf(" ------- Welcome -------- \r\n");

    spi_get_capabilities(spi_get_module(SPI_SLAVE_MOSI,
                                        SPI_SLAVE_MISO,
                                        SPI_SLAVE_CLK),
                         SPI_SLAVE_SS,
                         &capabilities);

    spi_init(&spi_slave, true, SPI_SLAVE_MOSI, SPI_SLAVE_MISO, SPI_SLAVE_CLK, SPI_SLAVE_SS);

    while (true) {
        spi_format(&spi_slave, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

        config_test_case_t config = {0};

        led1 = 1;
        count = spi_transfer(&spi_slave, NULL, 0, &config, CONFIG_LEN, (void*)&fill_symbol_8);
        led1 = 0;

        Timer tim;

        tim.reset();
        tim.start();

        printf("TEST CASE CONFIGURATION\r\n");
        printf("symbol_size: %lu\r\n", (uint32_t)config.symbol_size);
        printf("spi_mode: %lu\r\n", (uint32_t)config.mode);
        printf("bit_ordering: %lu\r\n", (uint32_t)config.bit_ordering);
        printf("freq: %lu\r\n", (uint32_t)config.freq_hz);
        printf("master tx cnt: %lu\r\n", (uint32_t)config.master_tx_cnt);
        printf("master rx cnt: %lu\r\n", (uint32_t)config.master_rx_cnt);
        printf("slave tx cnt: %lu\r\n", (uint32_t)config.slave_tx_cnt);
        printf("slave rx cnt: %lu\r\n", (uint32_t)config.slave_rx_cnt);
        printf("master tx defined: %lu\r\n", (uint32_t)config.master_tx_defined);
        printf("master rx defined: %lu\r\n", (uint32_t)config.master_rx_defined);
        printf("slave tx defined: %lu\r\n", (uint32_t)config.slave_tx_defined);
        printf("slave rx defined: %lu\r\n", (uint32_t)config.slave_rx_defined);
        printf("auto ss: %lu\r\n", (uint32_t)config.slave_rx_defined);
        printf("full duplex: %lu\r\n", (uint32_t)config.full_duplex);

        printf("log time: %lu [us]\r\n", (uint32_t)tim.read_us());
        printf("---\r\n");

        tim.stop();



        if (capabilities.word_length & ((uint32_t)1 << ((uint32_t)config.symbol_size - 1)) &&
            config.freq_hz >= capabilities.minimum_frequency &&
            config.freq_hz <= capabilities.maximum_frequency) {

            status = 0x00;

            led1 = 1;
            count = spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, s_rx_buff_uint8, 1, (void*) &fill_symbol_8);
            led1 = 0;

            spi_format(&spi_slave, config.symbol_size, (spi_mode_t)config.mode, (spi_bit_ordering_t)config.bit_ordering);

            init_buffers(&config);

            led1 = 1;
            count = spi_transfer(&spi_slave, p_tx_buf, config.slave_tx_cnt, p_rx_buf, config.slave_rx_cnt, (void*) p_fill_sym);
            led1 = 0;

            /* Send data received from master in the previous transmission. */

            if (config.symbol_size <= 8) {
                if (p_tx_buf && p_rx_buf) {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint8));
                }
            } else if (config.symbol_size <= 16) {
                if (p_tx_buf && p_rx_buf) {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint16));
                }
            } else {
                if (p_tx_buf && p_rx_buf) {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint32));
                }
            }
            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);
        } else {
            led1 = 1;
            status = 0x01;
            count = spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, NULL, 0, (void*) &fill_symbol_8);
            led1 = 0;
        }
    }

    spi_free(&spi_slave);
}

