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
#define SPI_MASTER_MOSI      PTD2
#define SPI_MASTER_MISO      PTD3
#define SPI_MASTER_SS        PTD0
#define SPI_MASTER_CLK       PTD1

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)

static spi_t spi_master = { 0 };

#define DEFAULT_CFG 0

#define CONFIG_LEN 8
#define CONFIG_STATUS_LEN 1

#define TEST_SYM_CNT 5

static uint8_t rx_pattern_8[TEST_SYM_CNT] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint16_t rx_pattern_16[TEST_SYM_CNT] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
static uint32_t rx_pattern_32[TEST_SYM_CNT] = {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555};

static uint8_t tx_pattern_8[TEST_SYM_CNT] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static uint16_t tx_pattern_16[TEST_SYM_CNT] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
static uint32_t tx_pattern_32[TEST_SYM_CNT] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};

static uint8_t m_config_buf[CONFIG_LEN];
static uint8_t m_status[CONFIG_STATUS_LEN];
static uint8_t m_tx_buff_uint8[TEST_SYM_CNT];
static uint8_t m_rx_buff_uint8[TEST_SYM_CNT];
static uint16_t m_tx_buff_uint16[TEST_SYM_CNT];
static uint16_t m_rx_buff_uint16[TEST_SYM_CNT];
static uint32_t m_tx_buff_uint32[TEST_SYM_CNT];
static uint32_t m_rx_buff_uint32[TEST_SYM_CNT];

static const uint8_t fill_symbol_8 = (uint8_t)0xFF;
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


static void clear_buf(void * addr, uint32_t size)
{
    char *p_char = (char*)addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = 0;
    }
}

void test_transfer_master()
{
    int32_t count;
    int8_t status;
    spi_capabilities_t capabilities =
        { 0 };

    spi_get_capabilities(spi_get_module(SPI_MASTER_MOSI,
                                        SPI_MASTER_MISO,
                                        SPI_MASTER_CLK),
                         NC,
                         &capabilities);


    spi_init(&spi_master, false, SPI_MASTER_MOSI, SPI_MASTER_MISO, SPI_MASTER_CLK, SPI_MASTER_SS);

    for (uint32_t tc_id = 10; tc_id < (sizeof(test_cases) / sizeof(config_test_case_t)); tc_id++) {

        printf("---> Test Case: %u \n", tc_id);

        if (capabilities.word_length & (1 << (test_cases[tc_id].symbol_size - 1))) {

            spi_transmission_config_t config =
            { 0x0C,
              test_cases[tc_id].symbol_size,
              test_cases[tc_id].mode,
              test_cases[tc_id].bit_ordering,
              test_cases[tc_id].freq_hz };

            spi_format(&spi_master,
                       test_cases[DEFAULT_CFG].symbol_size,
                       test_cases[DEFAULT_CFG].mode,
                       test_cases[DEFAULT_CFG].bit_ordering);
            spi_frequency(&spi_master, test_cases[DEFAULT_CFG].freq_hz);

            wait_ms(100);

            count = spi_transfer(&spi_master, &config, CONFIG_LEN, NULL, 0, (void*) &fill_symbol_8);

            //printf("SPI count: %u \n", count);

            wait_ms(100);

            count = spi_transfer(&spi_master, NULL, 0, &status, CONFIG_STATUS_LEN, (void*) &fill_symbol_8);

            //printf("SPI count: %u \n", count);
            printf("Config status: %X \n", (uint32_t)status);

            if (status == 0x00) {
                spi_format(&spi_master,
                           test_cases[tc_id].symbol_size,
                           test_cases[tc_id].mode,
                           test_cases[tc_id].bit_ordering);
                spi_frequency(&spi_master, test_cases[tc_id].freq_hz);

                if (test_cases[tc_id].symbol_size <= 8) {
                    p_tx_buf = m_tx_buff_uint8;
                    p_rx_buf = m_rx_buff_uint8;
                    p_fill_sym = (void*) &fill_symbol_8;
                    memcpy(m_tx_buff_uint8, tx_pattern_8, sizeof(m_tx_buff_uint8));
                } else if (test_cases[tc_id].symbol_size <= 16) {
                    p_tx_buf = m_tx_buff_uint16;
                    p_rx_buf = m_rx_buff_uint16;
                    p_fill_sym = (void*) &fill_symbol_16;
                } else {
                    p_tx_buf = m_tx_buff_uint32;
                    p_rx_buf = m_rx_buff_uint32;
                    p_fill_sym = (void*) &fill_symbol_32;
                }

                wait_ms(100);

                count = spi_transfer(&spi_master, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

                printf("SPI count: %u \n", count);
                printf("master tx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_tx_buff_uint8[0], m_tx_buff_uint8[1], m_tx_buff_uint8[2],
                        m_tx_buff_uint8[3], m_tx_buff_uint8[4]);
                printf("master rx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_rx_buff_uint8[0], m_rx_buff_uint8[1], m_rx_buff_uint8[2],
                        m_rx_buff_uint8[3], m_rx_buff_uint8[4]);

                if (test_cases[tc_id].symbol_size <= 8) {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(m_tx_buff_uint8));
                } else if (test_cases[tc_id].symbol_size <= 16) {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(m_tx_buff_uint16));
                } else {
                    memcpy(p_tx_buf, p_rx_buf, sizeof(m_tx_buff_uint32));
                }

                wait_ms(100);

                count = spi_transfer(&spi_master, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

                printf("SPI count: %u \n", count);
                printf("master tx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_tx_buff_uint8[0], m_tx_buff_uint8[1], m_tx_buff_uint8[2],
                        m_tx_buff_uint8[3], m_tx_buff_uint8[4]);
                printf("master rx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_rx_buff_uint8[0], m_rx_buff_uint8[1], m_rx_buff_uint8[2],
                        m_rx_buff_uint8[3], m_rx_buff_uint8[4]);

            } else {
                printf("Not supported by slave. \n");
            }
        } else {
            printf("Not supported by master. \n");
        }

        break;

    }

    spi_free(&spi_master);
}

void test_transfer_master_single()
{
    int32_t count;
    int8_t status;
    int32_t tc_id = 12;

    spi_init(&spi_master, false, SPI_MASTER_MOSI, SPI_MASTER_MISO, SPI_MASTER_CLK, SPI_MASTER_SS);

    spi_format(&spi_master,
               test_cases[tc_id].symbol_size,
               test_cases[tc_id].mode,
               test_cases[tc_id].bit_ordering);
    spi_frequency(&spi_master, test_cases[tc_id].freq_hz);

    wait(2);

    p_tx_buf = m_tx_buff_uint8;
    p_rx_buf = m_rx_buff_uint8;
    p_fill_sym = (void*) &fill_symbol_8;
    memcpy(m_tx_buff_uint8, tx_pattern_8, sizeof(m_tx_buff_uint8));

    wait_ms(100);

    count = spi_transfer(&spi_master, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

    printf("SPI count: %u \n", count);
    printf("master tx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_tx_buff_uint8[0], m_tx_buff_uint8[1], m_tx_buff_uint8[2],
            m_tx_buff_uint8[3], m_tx_buff_uint8[4]);
    printf("master rx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_rx_buff_uint8[0], m_rx_buff_uint8[1], m_rx_buff_uint8[2],
            m_rx_buff_uint8[3], m_rx_buff_uint8[4]);

    memcpy(p_tx_buf, p_rx_buf, sizeof(m_tx_buff_uint8));

    wait_ms(100);

    count = spi_transfer(&spi_master, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);

    printf("SPI count: %u \n", count);
    printf("master tx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_tx_buff_uint8[0], m_tx_buff_uint8[1], m_tx_buff_uint8[2],
            m_tx_buff_uint8[3], m_tx_buff_uint8[4]);
    printf("master rx: 0x%X 0x%X 0x%X 0x%X 0x%X \n", m_rx_buff_uint8[0], m_rx_buff_uint8[1], m_rx_buff_uint8[2],
            m_rx_buff_uint8[3], m_rx_buff_uint8[4]);

    spi_free(&spi_master);
}



utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(40, "spi_sync");
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
    //Case("SPI - master transfer test", test_transfer_master, greentea_failure_handler),
    Case("SPI - single master transfer test", test_transfer_master_single, greentea_failure_handler)
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
