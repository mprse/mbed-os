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

DigitalOut led1(LED2);

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

#define CONFIG_LEN 8
#define CONFIG_STATUS_LEN 1

#define TEST_SYM_CNT 5

static uint8_t tx_pattern_8[TEST_SYM_CNT] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint16_t tx_pattern_16[TEST_SYM_CNT] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
static uint32_t tx_pattern_32[TEST_SYM_CNT] = {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555};

static uint8_t rx_pattern_8[TEST_SYM_CNT] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static uint16_t rx_pattern_16[TEST_SYM_CNT] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
static uint32_t rx_pattern_32[TEST_SYM_CNT] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};

static uint8_t s_config_buf[CONFIG_LEN];
static uint8_t s_status[CONFIG_STATUS_LEN];
static uint8_t s_tx_buff_uint8[TEST_SYM_CNT];
static uint8_t s_rx_buff_uint8[TEST_SYM_CNT];
static uint16_t s_tx_buff_uint16[TEST_SYM_CNT];
static uint16_t s_rx_buff_uint16[TEST_SYM_CNT];
static uint32_t s_tx_buff_uint32[TEST_SYM_CNT];
static uint32_t s_rx_buff_uint32[TEST_SYM_CNT];

static const uint8_t fill_symbol_8 = (uint8_t)0x55;
static const uint8_t fill_symbol_16 = (uint8_t)0xFFFF;
static const uint8_t fill_symbol_32 = (uint8_t)0xFFFFFFFF;

static void *p_tx_buf;
static void *p_rx_buf;
static void *p_fill_sym;

typedef struct {
    uint8_t preamble;
    uint8_t symbol_size;
    uint8_t spi_mode;
    uint8_t bit_ordering;
    uint32_t freq;
} spi_transmission_config_t;

static void clear_buf(void * addr, uint32_t size)
{
    char *p_char = (char*)addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = 0;
    }
}

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
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},

        /* symbol size testing */
/* 01 */{0, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 02 */{1, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 03 */{7, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 04 */{9, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 05 */{15, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 06 */{16, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 07 */{17, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 08 */{31, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 09 */{32, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* mode testing */
/* 10 */{8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 11 */{8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
/* 12 */{8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* bit ordering testing */
/* 13 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_1MHZ},
        /* freq testing */
/* 14 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_200KHZ},
/* 15 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST, FREQ_2MHZ},
};


static void init_buffers(uint32_t symbol_size)
{
    if (symbol_size <= 8) {
        p_tx_buf = s_tx_buff_uint8;
        p_rx_buf = s_rx_buff_uint8;
        p_fill_sym = (void*) &fill_symbol_8;
        memcpy(s_tx_buff_uint8, tx_pattern_8, sizeof(s_tx_buff_uint8));
    } else if (symbol_size <= 16) {
        p_tx_buf = s_tx_buff_uint16;
        p_rx_buf = s_rx_buff_uint16;
        p_fill_sym = (void*) &fill_symbol_16;
    } else {
        p_tx_buf = s_tx_buff_uint32;
        p_rx_buf = s_rx_buff_uint32;
        p_fill_sym = (void*) &fill_symbol_32;
    }

}

int main()
{
    int32_t count;
    int8_t status;
    spi_capabilities_t capabilities =
        { 0 };

    printf("Welcome\r\n");

    spi_get_capabilities(spi_get_module(SPI_SLAVE_MOSI,
                                        SPI_SLAVE_MISO,
                                        SPI_SLAVE_CLK),
                         SPI_SLAVE_SS,
                         &capabilities);

    spi_init(&spi_slave, true, SPI_SLAVE_MOSI, SPI_SLAVE_MISO, SPI_SLAVE_CLK, SPI_SLAVE_SS);

    while (true) {
        spi_format(&spi_slave, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

        spi_transmission_config_t config = {0};

//        printf("1 \r\n");
        count = spi_transfer(&spi_slave, NULL, 0, &config, CONFIG_LEN, (void*)&fill_symbol_8);

        printf("config preamble: %u\r\n", (uint32_t)config.preamble);
        printf("config symbol_size: %u\r\n", (uint32_t)config.symbol_size);
        printf("config spi_mode: %u\r\n", (uint32_t)config.spi_mode);
        printf("config bit_ordering: %u\r\n", (uint32_t)config.bit_ordering);
        printf("config freq: %u\r\n", (uint32_t)config.freq);
        printf("---\n");

        if (capabilities.word_length & ((uint32_t)1 << ((uint32_t)config.symbol_size - 1)) &&
            config.freq >= capabilities.minimum_frequency &&
            config.freq <= capabilities.maximum_frequency) {

            //printf("sending ok status\r\n");

            status = 0x00;

            count = spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, s_rx_buff_uint8, 1, (void*) &fill_symbol_8);

            spi_format(&spi_slave, config.symbol_size, (spi_mode_t)config.spi_mode, (spi_bit_ordering_t)config.bit_ordering);

            /* Test 1: RX == TX (send 5 symbols, read 5 symbols). */

            init_buffers(config.symbol_size);

//            printf("3 \r\n");
            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

            /* Send data received from master in the previous transmission. */

            if (config.symbol_size <= 8) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint8));
            } else if (config.symbol_size <= 16) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint16));
            } else {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint32));
            }
            //printf("4 \r\n");
            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

            /* Test 2: Master: TX > RX (send 3 symbols, receive 5 symbols). */

            init_buffers(config.symbol_size);

            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

            /* Send data received from master in the previous transmission. */

            if (config.symbol_size <= 8) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint8));
            } else if (config.symbol_size <= 16) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint16));
            } else {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint32));
            }

            //printf("RX: 0x%X 0x%X 0x%X 0x%X 0x%X \n", s_rx_buff_uint8[0], s_rx_buff_uint8[1], s_rx_buff_uint8[2], s_rx_buff_uint8[3], s_rx_buff_uint8[4]);

            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

            /* Test 2: Master: TX > RX (send 3 symbols, receive 5 symbols). */

            init_buffers(config.symbol_size);

            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

            /* Send data received from master in the previous transmission. */

            if (config.symbol_size <= 8) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint8));
            } else if (config.symbol_size <= 16) {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint16));
            } else {
                memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint32));
            }

            //printf("RX: 0x%X 0x%X 0x%X 0x%X 0x%X \n", s_rx_buff_uint8[0], s_rx_buff_uint8[1], s_rx_buff_uint8[2], s_rx_buff_uint8[3], s_rx_buff_uint8[4]);

            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

        } else {
            //printf("sending not supported status:\r\n");
            status = 0x01;
//            printf("5 \r\n");
            count = spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, NULL, 0, (void*) &fill_symbol_8);

            if (config.symbol_size <= 8) {
                p_tx_buf = s_tx_buff_uint8;
                p_rx_buf = s_rx_buff_uint8;
                p_fill_sym = (void*) &fill_symbol_8;
                memcpy(s_tx_buff_uint8, tx_pattern_8, sizeof(s_tx_buff_uint8));
            } else if (config.symbol_size <= 16) {
                p_tx_buf = s_tx_buff_uint16;
                p_rx_buf = s_rx_buff_uint16;
                p_fill_sym = (void*) &fill_symbol_16;
            } else {
                p_tx_buf = s_tx_buff_uint32;
                p_rx_buf = s_rx_buff_uint32;
                p_fill_sym = (void*) &fill_symbol_32;
            }

            count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);
        }
    }

    spi_free(&spi_slave);
}

int main_single()
{
    int32_t count;
    int8_t status;
    int32_t tc_id = 12;

    //printf("Welcome\r\n");

    spi_init(&spi_slave, true, SPI_SLAVE_MOSI, SPI_SLAVE_MISO, SPI_SLAVE_CLK, SPI_SLAVE_SS);

    spi_format(&spi_slave,
               test_cases[tc_id].symbol_size,
               test_cases[tc_id].mode,
               test_cases[tc_id].bit_ordering);

    p_tx_buf = s_tx_buff_uint8;
    p_rx_buf = s_rx_buff_uint8;
    p_fill_sym = (void*) &fill_symbol_8;
    memcpy(s_tx_buff_uint8, tx_pattern_8, sizeof(s_tx_buff_uint8));

    //printf("1 \r\n");
    count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

    memcpy(p_tx_buf, p_rx_buf, sizeof(s_tx_buff_uint8));
    //printf("2 \r\n");
    count = spi_transfer(&spi_slave, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

    spi_free(&spi_slave);
}
