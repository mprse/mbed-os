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
#include "spi_test_common.h"

using namespace utest::v1;

#if (IS_MASTER)
/* Array witch test cases which represents different SPI configurations for testing. */
static config_test_case_t test_cases[] = {
        /* default config: 8 bit symbol\sync mode\full duplex\clock idle low\sample on the first clock edge\MSB first\100 KHz clock\manual SS handling */
/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE    , SPI_BIT_ORDERING_MSB_FIRST, 1000000        , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* symbol size testing */
/* 01 */{1  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 02 */{7  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 03 */{9  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 04 */{15 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 05 */{16 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 06 */{17 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 07 */{31 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 08 */{32 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* mode testing */
/* 09 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 10 */{8  , SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 11 */{8  , SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* bit ordering testing */
/* 12 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_LSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* freq testing */
/* 13 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 14 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_2MHZ    , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 15 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_MIN     , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
/* 16 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_MAX     , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* master: TX > RX */
/* 17 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT-2, TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* master: TX < RX */
/* 18 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT-2 , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* slave: TX > RX */
/* 19 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT-2, true , true , true , true , false , FULL_DUPLEX     , true },
        /* slave: TX < RX */
/* 20 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT-2, TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , true },
        /* master tx buffer undefined */
/* 21 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , false, true , true , true , false , FULL_DUPLEX     , true },
        /* master rx buffer undefined */
/* 22 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , false, true , true , false , FULL_DUPLEX     , true },
        /* slave tx buffer undefined */
/* 23 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , false, true , false , FULL_DUPLEX     , true },
        /* slave rx buffer undefined */
/* 24 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , false, false , FULL_DUPLEX     , true },
        /* manual ss hadling by master */
/* 25 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true  , FULL_DUPLEX     , true },
        /* half duplex mode  */
/* 26 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , HALF_DUPLEX_MISO, true },
/* 27 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , HALF_DUPLEX_MOSI, true },
        /* async mode */
/* 28 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE    , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false , FULL_DUPLEX     , false},

};

/* Function compares given buffers and returns true when equal, false otherwise.
 * In case when buffer is undefined (NULL) function returns true. */
static bool check_buffers(void *p_pattern, void *p_buffer, uint32_t size)
{
    const char * p_byte_pattern = (const char*) p_pattern;
    const char * p_byte_buffer = (const char*) p_buffer;

    if (p_buffer == NULL || p_pattern == NULL) {
        return true;
    }

    while (size) {
        if (*p_byte_pattern != *p_byte_buffer) {
            //printf("comp: 0x%X 0x%X \n", *p_byte_pattern, *p_byte_buffer);

            //printf("pattern: 0x%2X 0x%2X 0x%2X 0x%2X 0x%2X \n", p_byte_pattern[0], p_byte_pattern[1], p_byte_pattern[2], p_byte_pattern[3], p_byte_pattern[4]);
            //printf("buffer : 0x%2X 0x%2X 0x%2X 0x%2X 0x%2X \n", p_byte_buffer[0], p_byte_buffer[1], p_byte_buffer[2], p_byte_buffer[3], p_byte_buffer[4]);
            return false;
        }
        //printf("comp: 0x%X 0x%X \n", *p_byte_pattern, *p_byte_buffer);
        p_byte_pattern++;
        p_byte_buffer++;
        size--;
    }

    return true;
}

/* Function initialises RX, TX buffers before transmission. */
template<typename T>
static void init_transmission_buffers(uint32_t tc_id, T *p_tx_pattern, T *p_rx1_pattern, T *p_rx2_pattern, T *p_tx_buff, T *p_rx_buff, T *p_fill_symbol)
{
    /* Default patterns for TX/RX buffers. */
    set_buffer(&p_tx_pattern[0], sizeof(T), 0xAA);
    set_buffer(&p_tx_pattern[1], sizeof(T), 0xBB);
    set_buffer(&p_tx_pattern[2], sizeof(T), 0xCC);
    set_buffer(&p_tx_pattern[3], sizeof(T), 0xDD);
    set_buffer(&p_tx_pattern[4], sizeof(T), 0xEE);

    set_buffer(&p_rx1_pattern[0], sizeof(T), 0x11);
    set_buffer(&p_rx1_pattern[1], sizeof(T), 0x22);
    set_buffer(&p_rx1_pattern[2], sizeof(T), 0x33);
    set_buffer(&p_rx1_pattern[3], sizeof(T), 0x44);
    set_buffer(&p_rx1_pattern[4], sizeof(T), 0x55);

    set_buffer(&p_rx2_pattern[0], sizeof(T), 0xAA);
    set_buffer(&p_rx2_pattern[1], sizeof(T), 0xBB);
    set_buffer(&p_rx2_pattern[2], sizeof(T), 0xCC);
    set_buffer(&p_rx2_pattern[3], sizeof(T), 0xDD);
    set_buffer(&p_rx2_pattern[4], sizeof(T), 0xEE);

    set_buffer(p_fill_symbol, sizeof(T), 0xFF);

    /* Exception: master TX > master RX . */
    if (test_cases[tc_id].master_tx_cnt > test_cases[tc_id].master_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);
    }
    /* Exception: master TX < master RX . */
    if (test_cases[tc_id].master_tx_cnt < test_cases[tc_id].master_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave TX > slave RX . */
    if (test_cases[tc_id].slave_tx_cnt > test_cases[tc_id].slave_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);
    }
    /* Exception: slave TX < slave RX . */
    if (test_cases[tc_id].slave_tx_cnt < test_cases[tc_id].slave_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: master TX buffer undefined . */
    if (!test_cases[tc_id].master_tx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: master RX buffer undefined . */
    if (!test_cases[tc_id].master_rx_defined) {
        set_buffer(&p_rx1_pattern[0], sizeof(T), 0xAA);
        set_buffer(&p_rx1_pattern[1], sizeof(T), 0xBB);
        set_buffer(&p_rx1_pattern[2], sizeof(T), 0xCC);
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xDD);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xEE);
    }

    /* Exception: slave TX buffer undefined . */
    if (!test_cases[tc_id].slave_tx_defined) {
        set_buffer(&p_rx1_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xFF);

        set_buffer(&p_rx2_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave RX buffer undefined . */
    if (!test_cases[tc_id].slave_rx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0x11);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0x22);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0x33);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x44);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x55);
    }

    /* Handle symbol size. */
    T sym_mask = ((1 << test_cases[tc_id].symbol_size) - 1);

    for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
        p_tx_pattern[i] = (p_tx_pattern[i] & sym_mask);
        p_rx1_pattern[i] = (p_rx1_pattern[i] & sym_mask);
        p_rx2_pattern[i] = (p_rx2_pattern[i] & sym_mask);
    }

    memcpy(p_tx_buff, p_tx_pattern, sizeof(T) * TEST_SYM_CNT);
    set_buffer(p_rx_buff, sizeof(T) * TEST_SYM_CNT, 0x00);
}

/* Function handles <ss> line if <ss> is specified.
 * When manual <ss> handling is selected <ss> is defined.
 */
static void handle_ss(DigitalOut * ss, bool select)
{
    if (ss) {
        if (select) {
            *ss = 0;
        } else {
            *ss = 1;
        }
    }
}

/* Function which perform transfer using specified config on the master side. */
template<typename T, const uint32_t tc_id>
void test_transfer_master()
{
    spi_t spi_master = { 0 };
    spi_capabilities_t capabilities = { 0 };
    uint32_t count;
    int8_t status[2];
    DigitalOut *ss_pin = NULL;
    PinName ss = SPI_SS;
    PinName miso = SPI_MISO;
    PinName mosi = SPI_MOSI;
    uint32_t clocked_symbols_1 = (test_cases[tc_id].slave_tx_cnt + test_cases[tc_id].slave_rx_cnt);
    uint32_t clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);

    T tx_pattern[TEST_SYM_CNT];
    T rx1_pattern[TEST_SYM_CNT];
    T rx2_pattern[TEST_SYM_CNT];
    T tx_buff[TEST_SYM_CNT];
    T rx_buff[TEST_SYM_CNT];
    T fill_symbol;

    void *p_tx_buff = tx_buff;
    void *p_rx_buff = rx_buff;

    if (!test_cases[tc_id].master_tx_defined) {
        p_tx_buff = NULL;
    }

    if (!test_cases[tc_id].master_rx_defined) {
        p_rx_buff = NULL;
    }

    spi_get_capabilities(spi_get_module(SPI_MOSI,
    SPI_MISO,
    SPI_CLK), NC, &capabilities);

    /* Adapt Full Duplex/Half Duplex settings. */
    switch (test_cases[tc_id].duplex)
    {
        case HALF_DUPLEX_MOSI:
            miso = NC;
            break;

        case HALF_DUPLEX_MISO:
            mosi = NC;
            break;

        default:
            clocked_symbols_1 = TEST_SYM_CNT;
            clocked_symbols_2 = TEST_SYM_CNT;
            break;
    }

    /* Adapt manual/auto SS handling by master. */
    if (!test_cases[tc_id].auto_ss) {
        ss_pin = new DigitalOut(SPI_SS);
        ss = NC;
        *ss_pin = 1;
    }

    spi_init(&spi_master, false, mosi, miso, SPI_CLK, ss);

    /* Continue if master can handle the config, otherwise skip it. */
    if (check_capabilities(&capabilities, test_cases[tc_id].symbol_size, false, test_cases[tc_id].duplex)) {

        /* Adapt min/max frequency for testing based of capabilities. */
        switch (test_cases[tc_id].freq_hz)
        {
            case FREQ_MIN:
                test_cases[tc_id].freq_hz = capabilities.minimum_frequency;
                break;

            case FREQ_MAX:
                test_cases[tc_id].freq_hz = capabilities.maximum_frequency;
                break;

            default:

                break;
        }

        /* Use default format to transfer test configuration to slave. Default format
         * should be handled by all SPI devices. */
        spi_format(&spi_master, test_cases[DEFAULT_CFG].symbol_size, test_cases[DEFAULT_CFG].mode, test_cases[DEFAULT_CFG].bit_ordering);

        spi_frequency(&spi_master, test_cases[DEFAULT_CFG].freq_hz);

        /* Send config to slave. Slave will return status indicating if given config can be
         * handled by slave.
         */
        uint8_t buf[CONFIG_LEN];
        wait_before_transmission();
        handle_ss(ss_pin, true);
        count = spi_transfer(&spi_master, &test_cases[tc_id], CONFIG_LEN, &buf, CONFIG_LEN, (void*) &fill_symbol);
        handle_ss(ss_pin, false);

        wait_before_transmission();
        handle_ss(ss_pin, true);
        count = spi_transfer(&spi_master, &buf, CONFIG_STATUS_LEN, status, CONFIG_STATUS_LEN, (void*) &fill_symbol);
        handle_ss(ss_pin, false);

        /* Continue if slave can handle the config, otherwise skip it. */
        if (status[0] == CONFIG_STATUS_OK) {
            spi_format(&spi_master, test_cases[tc_id].symbol_size, test_cases[tc_id].mode, test_cases[tc_id].bit_ordering);

            spi_frequency(&spi_master, test_cases[tc_id].freq_hz);

            init_transmission_buffers<T>(tc_id, &tx_pattern[0], &rx1_pattern[0], &rx2_pattern[0], &tx_buff[0], &rx_buff[0], &fill_symbol);

            dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

            wait_before_transmission();
            handle_ss(ss_pin, true);
            count = sync_async_transfer(&spi_master, p_tx_buff, test_cases[tc_id].master_tx_cnt, p_rx_buff, test_cases[tc_id].master_rx_cnt, (void*) &fill_symbol, test_cases[tc_id].sync);
            handle_ss(ss_pin, false);

            dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

            TEST_ASSERT_EQUAL(true, check_buffers(rx1_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT));
            TEST_ASSERT_EQUAL(true, check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT));
            TEST_ASSERT_EQUAL(clocked_symbols_1, count);

            /* Init TX buffer with data received from slave. */
            if (p_tx_buff && p_rx_buff) {
                memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
                memcpy(tx_pattern, p_rx_buff, sizeof(tx_buff));
            }

            set_buffer(rx_buff, sizeof(rx_buff), 0x00);

            dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

            wait_before_transmission();
            handle_ss(ss_pin, true);
            count = sync_async_transfer(&spi_master, tx_buff, TEST_SYM_CNT, rx_buff, TEST_SYM_CNT, (void*) &fill_symbol, test_cases[tc_id].sync);
            handle_ss(ss_pin, false);

            dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

            TEST_ASSERT_EQUAL(true, check_buffers(rx2_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT));
            TEST_ASSERT_EQUAL(true, check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT));
            TEST_ASSERT_EQUAL(clocked_symbols_2, count);

        } else if (status[0] == CONFIG_STATUS_NOT_SUPPORTED) {
            TEST_SKIP_MESSAGE("Config not supported by slave. Skipping. \n");
        } else {
            TEST_ASSERT_TRUE_MESSAGE(false, "Invalid configuration status. Communication error!");
        }
    } else {
        TEST_SKIP_MESSAGE("Config not supported by master. Skipping. \n");
    }

    spi_free(&spi_master);
}

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(40, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("SPI master-slave sync com - default config", test_transfer_master<uint8_t, 0>),
#if 1
    Case("SPI master-slave sync com - symbol size: 1", test_transfer_master<uint8_t, 1>),
    Case("SPI master-slave sync com - symbol size: 7", test_transfer_master<uint8_t, 2>),
    Case("SPI master-slave sync com - symbol size: 9", test_transfer_master<uint16_t, 3>),
    Case("SPI master-slave sync com - symbol size: 15", test_transfer_master<uint16_t, 4>),
    Case("SPI master-slave sync com - symbol size: 16", test_transfer_master<uint16_t, 5>),
    Case("SPI master-slave sync com - symbol size: 17", test_transfer_master<uint32_t, 6>),
    Case("SPI master-slave sync com - symbol size: 31", test_transfer_master<uint32_t, 7>),
    Case("SPI master-slave sync com - symbol size: 32", test_transfer_master<uint32_t, 8>),
    Case("SPI master-slave sync com - mode: idle low, sample second edge", test_transfer_master<uint8_t, 9>),
    Case("SPI master-slave sync com - mode: idle high, sample first edge", test_transfer_master<uint8_t, 10>),
    Case("SPI master-slave sync com - mode: idle high, sample second edge", test_transfer_master<uint8_t, 11>),
    Case("SPI master-slave sync com - bit ordering: LSB first", test_transfer_master<uint8_t, 12>),
    Case("SPI master-slave sync com - freq testing: 200 KHz", test_transfer_master<uint8_t, 13>),
    Case("SPI master-slave sync com - freq testing: 2 MHz", test_transfer_master<uint8_t, 14>),
    Case("SPI master-slave sync com - freq testing: min defined", test_transfer_master<uint8_t, 15>),
    Case("SPI master-slave sync com - freq testing: max defined", test_transfer_master<uint8_t, 16>),
    Case("SPI master-slave sync com - master: TX > RX", test_transfer_master<uint8_t, 17>),
    Case("SPI master-slave sync com - master: TX < RX", test_transfer_master<uint8_t, 18>),
    Case("SPI master-slave sync com - slave: TX > RX", test_transfer_master<uint8_t, 19>),
    Case("SPI master-slave sync com - slave: TX < RX", test_transfer_master<uint8_t, 20>),
    Case("SPI master-slave sync com - master: TX undefined", test_transfer_master<uint8_t, 21>),
    Case("SPI master-slave sync com - master: RX undefined", test_transfer_master<uint8_t, 22>),
    Case("SPI master-slave sync com - slave: TX undefined", test_transfer_master<uint8_t, 23>),
    Case("SPI master-slave sync com - slave: RX undefined", test_transfer_master<uint8_t, 24>),
    Case("SPI master-slave sync com - master: manual ss", test_transfer_master<uint8_t, 25>),
    Case("SPI master-slave sync com - half duplex (MOSI)", test_transfer_master<uint8_t, 26>),
    Case("SPI master-slave sync com - half duplex (MISO)", test_transfer_master<uint8_t, 27>),
    Case("SPI master-slave async com - default config", test_transfer_master<uint8_t, 28>),
#endif
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
#else /* SLAVE */
/* Function which perform transfer using specified config on the slave side. */
template<typename T>
void slave_transfer(spi_t *obj, config_test_case_t *config)
{
    uint32_t count;
    T tx_buff[TEST_SYM_CNT];
    T rx_buff[TEST_SYM_CNT];
    T fill_symbol;
    uint32_t clocked_symbols_1 = TEST_SYM_CNT;
    uint32_t clocked_symbols_2 = TEST_SYM_CNT;

    if (config->duplex != FULL_DUPLEX) {
        clocked_symbols_1 = (config->slave_tx_cnt + config->slave_rx_cnt);
        clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);
    }

    set_buffer(&tx_buff[0], sizeof(T), 0x11);
    set_buffer(&tx_buff[1], sizeof(T), 0x22);
    set_buffer(&tx_buff[2], sizeof(T), 0x33);
    set_buffer(&tx_buff[3], sizeof(T), 0x44);
    set_buffer(&tx_buff[4], sizeof(T), 0x55);

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    set_buffer(&fill_symbol, sizeof(fill_symbol), 0xFF);

    void *p_tx_buff = tx_buff;
    void *p_rx_buff = rx_buff;

    if (!config->slave_tx_defined) {
        p_tx_buff = NULL;
    }

    if (!config->slave_rx_defined) {
        p_rx_buff = NULL;
    }

    wait_before_transmission();
    count = sync_async_transfer(obj, p_tx_buff, config->slave_tx_cnt, p_rx_buff, config->slave_rx_cnt, &fill_symbol, config->sync);

    TEST_ASSERT_EQUAL_MESSAGE(clocked_symbols_1, count, "Transmission 1: Invalid number of clocked symbols.\r\n");

    /* Send data received from master in the previous transmission. */
    if (p_tx_buff && p_rx_buff) {
        memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
    }

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    wait_before_transmission();
    count = sync_async_transfer(obj, p_tx_buff, TEST_SYM_CNT, p_rx_buff, TEST_SYM_CNT, &fill_symbol, config->sync);

    TEST_ASSERT_EQUAL_MESSAGE(clocked_symbols_2, count, "Transmission 2: Invalid number of clocked symbols.\r\n");
}

int main()
{
    spi_t spi_slave = { 0 };
    int8_t status[2];
    uint8_t fill_symbol = 0xFF;
    spi_capabilities_t capabilities = { 0 };
    uint8_t buf[CONFIG_LEN];

    printf(" ------- Slave Welcome -------- \r\n");

#if DEBUG
    printf("Debug mode... \r\n");
#else
    printf("Normal mode... \r\n");
#endif

    spi_get_capabilities(spi_get_module(SPI_MOSI,
                                        SPI_MISO,
                                        SPI_CLK),
                         SPI_SS,
                         &capabilities);

    while (true) {
        spi_init(&spi_slave, true, SPI_MOSI, SPI_MISO, SPI_CLK, SPI_SS);

        spi_format(&spi_slave, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

        config_test_case_t config = { 0 };

        wait_before_transmission();
        spi_transfer(&spi_slave, &buf, CONFIG_LEN, &config, CONFIG_LEN, &fill_symbol);

        dump_config(&config);

        if (check_capabilities(&capabilities, config.symbol_size, true, config.duplex)) {
            status[0] = CONFIG_STATUS_OK;
            PinName miso = SPI_MISO;
            PinName mosi = SPI_MOSI;

            wait_before_transmission();
            spi_transfer(&spi_slave, status, CONFIG_STATUS_LEN, &buf, CONFIG_STATUS_LEN, (void*) &fill_symbol);

            spi_free(&spi_slave);

            /* Adapt Full duplex/Half duplex settings. */
            switch (config.duplex)
            {
                case HALF_DUPLEX_MOSI:
                    miso = NC;
                    break;

                case HALF_DUPLEX_MISO:
                    mosi = NC;
                    break;

                default:

                    break;
            }
            spi_init(&spi_slave, true, mosi, miso, SPI_CLK, SPI_SS);

            spi_format(&spi_slave, config.symbol_size, (spi_mode_t) config.mode, (spi_bit_ordering_t) config.bit_ordering);

            if (config.symbol_size <= 8) {
                slave_transfer<uint8_t>(&spi_slave, &config);
            } else if (config.symbol_size <= 16) {
                slave_transfer<uint16_t>(&spi_slave, &config);
            } else {
                slave_transfer<uint32_t>(&spi_slave, &config);
            }
        } else {
            status[0] = CONFIG_STATUS_NOT_SUPPORTED;
            wait_before_transmission();
            spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, NULL, 0, &fill_symbol);
        }
        spi_free(&spi_slave);
    }
}
#endif
