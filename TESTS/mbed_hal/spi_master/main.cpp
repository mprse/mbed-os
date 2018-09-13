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
#define FREQ_MIN    (0)
#define FREQ_MAX    (0xFFFFFFFF)

DigitalIn button(SW3);
DigitalOut led1(LED1);

static spi_t spi_master = { 0 };

#define DEFAULT_CFG 0
#define CONFIG_START    (0xC0)
#define CONFIG_STATUS_OK 0x00
#define CONFIG_STATUS_NOT_SUPPORTED 0x01

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
    duplex_t duplex;
} config_test_case_t;

#define CONFIG_LEN (sizeof(config_test_case_t))
#define CONFIG_STATUS_LEN 1

static uint8_t tx_pattern_8[TEST_SYM_CNT] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static uint16_t tx_pattern_16[TEST_SYM_CNT] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
static uint32_t tx_pattern_32[TEST_SYM_CNT] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};

static uint8_t rx1_pattern_8[TEST_SYM_CNT] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint16_t rx1_pattern_16[TEST_SYM_CNT] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
static uint32_t rx1_pattern_32[TEST_SYM_CNT] = {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555};

static uint8_t rx2_pattern_8[TEST_SYM_CNT] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static uint16_t rx2_pattern_16[TEST_SYM_CNT] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
static uint32_t rx2_pattern_32[TEST_SYM_CNT] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};

static uint8_t tx_buff_uint8[TEST_SYM_CNT];
static uint8_t rx_buff_uint8[TEST_SYM_CNT];
static uint16_t tx_buff_uint16[TEST_SYM_CNT];
static uint16_t rx_buff_uint16[TEST_SYM_CNT];
static uint32_t tx_buff_uint32[TEST_SYM_CNT];
static uint32_t rx_buff_uint32[TEST_SYM_CNT];

static const uint8_t fill_symbol_8 = (uint8_t)0xFF;
static const uint8_t fill_symbol_16 = (uint8_t)0xFFFF;
static const uint8_t fill_symbol_32 = (uint8_t)0xFFFFFFFF;

static void *p_tx_buf;
static void *p_rx_buf;
static void *p_tx_pattern;
static void *p_rx1_pattern;
static void *p_rx2_pattern;
static void *p_fill_sym;
static uint32_t buff_size;

/* Array witch test cases which represents different SPI configs for testing. */
static config_test_case_t test_cases[] = {
        /* default config: 8 bit symbol\full duplex\clock idle low\sample on the first clock edge\ MSB first\1 MHz clock\automatic SS handling */

/* 00 */{8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE    , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ, TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* symbol size testing */
#if 0
/* 01 */{1  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 02 */{7  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 03 */{9  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 04 */{15 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 05 */{16 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 06 */{17 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 07 */{31 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 08 */{32 , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* mode testing */
/* 09 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 10 */{8  , SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 11 */{8  , SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* bit ordering testing */
/* 12 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_LSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* freq testing */
/* 13 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_200KHZ, TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 14 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_2MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 15 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_MIN   , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
/* 16 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_MAX   , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* master: TX > RX */
/* 17 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT-2, TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* master: TX < RX */
/* 18 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT-2 , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* slave: TX > RX */
/* 19 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT-2, true , true , true , true , true , FULL_DUPLEX  },
        /* slave: TX < RX */
/* 20 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT-2, TEST_SYM_CNT  , true , true , true , true , true , FULL_DUPLEX  },
        /* master tx buffer undefined */
/* 21 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , false, true , true , true , true , FULL_DUPLEX  },
        /* master rx buffer undefined */
/* 22 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , false, true , true , true , FULL_DUPLEX  },
        /* slave tx buffer undefined */
/* 23 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , false, true , true , FULL_DUPLEX  },
        /* slave rx buffer undefined */
/* 24 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , false, true , FULL_DUPLEX  },
#endif
        /* manual ss hadling by master */
/* 25 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , false, FULL_DUPLEX  },
        /* half duplex mode  */
/* 26 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , HALF_DUPLEX_MISO },
/* 27 */{8  , SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ  , TEST_SYM_CNT   , TEST_SYM_CNT  , TEST_SYM_CNT  , TEST_SYM_CNT  , true , true , true , true , true , HALF_DUPLEX_MOSI },

};


static void wait_until_button_pressed()
{
    while (button.read() == 1);
    while (button.read() == 0);
}

static void set_buffer(void * addr, uint32_t size, char val)
{
    if (addr == NULL) {
        return;
    }

    char *p_char = (char*)addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = val;
    }
}

static bool check_buffers(void *p_pattern, void *p_buffer, uint32_t size)
{
    const char * p_byte_pattern = (const char*) p_pattern;
    const char * p_byte_buffer = (const char*) p_buffer;

    if (p_buffer == NULL || p_pattern == NULL) {
        return true;
    }

    while(size) {
        if (*p_byte_pattern != *p_byte_buffer) {
            printf("comp: 0x%X 0x%X \n", *p_byte_pattern, *p_byte_buffer);

            printf("pattern: 0x%2X 0x%2X 0x%2X 0x%2X 0x%2X \n", p_byte_pattern[0], p_byte_pattern[1], p_byte_pattern[2], p_byte_pattern[3], p_byte_pattern[4]);
            printf("buffer : 0x%2X 0x%2X 0x%2X 0x%2X 0x%2X \n", p_byte_buffer[0], p_byte_buffer[1], p_byte_buffer[2], p_byte_buffer[3], p_byte_buffer[4]);
            return false;
        }
        printf("comp: 0x%X 0x%X \n", *p_byte_pattern, *p_byte_buffer);
        p_byte_pattern++;
        p_byte_buffer++;
        size--;
    }

    return true;
}

static void init_transmission_buffers(uint32_t tc_id)
{
    p_tx_pattern = NULL;
    p_rx1_pattern = NULL;
    p_rx2_pattern = NULL;
    p_tx_buf = NULL;
    p_rx_buf = NULL;
    p_fill_sym = NULL;

    /* Default patterns for TX/RX buffers. */
    tx_pattern_8[0] = 0xAA; tx_pattern_16[0] = 0xAAAA; tx_pattern_32[0] = 0xAAAAAAAA;
    tx_pattern_8[1] = 0xBB; tx_pattern_16[1] = 0xAAAA; tx_pattern_32[1] = 0xAAAAAAAA;
    tx_pattern_8[2] = 0xCC; tx_pattern_16[2] = 0xAAAA; tx_pattern_32[2] = 0xAAAAAAAA;
    tx_pattern_8[3] = 0xDD; tx_pattern_16[3] = 0xAAAA; tx_pattern_32[3] = 0xAAAAAAAA;
    tx_pattern_8[4] = 0xEE; tx_pattern_16[4] = 0xAAAA; tx_pattern_32[4] = 0xAAAAAAAA;

    rx1_pattern_8[0] = 0x11; rx1_pattern_16[0] = 0x1111; rx1_pattern_32[0] = 0x11111111;
    rx1_pattern_8[1] = 0x22; rx1_pattern_16[1] = 0x2222; rx1_pattern_32[1] = 0x22222222;
    rx1_pattern_8[2] = 0x33; rx1_pattern_16[2] = 0x3333; rx1_pattern_32[2] = 0x33333333;
    rx1_pattern_8[3] = 0x44; rx1_pattern_16[3] = 0x4444; rx1_pattern_32[3] = 0x44444444;
    rx1_pattern_8[4] = 0x55; rx1_pattern_16[4] = 0x5555; rx1_pattern_32[4] = 0x55555555;

    rx2_pattern_8[0] = 0xAA; rx2_pattern_16[0] = 0xAAAA; rx2_pattern_32[0] = 0x11111111;
    rx2_pattern_8[1] = 0xBB; rx2_pattern_16[1] = 0xBBBB; rx2_pattern_32[1] = 0x22222222;
    rx2_pattern_8[2] = 0xCC; rx2_pattern_16[2] = 0xCCCC; rx2_pattern_32[2] = 0x33333333;
    rx2_pattern_8[3] = 0xDD; rx2_pattern_16[3] = 0xDDDD; rx2_pattern_32[3] = 0x44444444;
    rx2_pattern_8[4] = 0xEE; rx2_pattern_16[4] = 0xEEEE; rx2_pattern_32[4] = 0x55555555;

    /* Exception: master TX < master RX . */
    if (test_cases[tc_id].master_tx_cnt < test_cases[tc_id].master_rx_cnt) {
        rx2_pattern_8[0] = 0xAA; rx2_pattern_16[0] = 0xAAAA; rx2_pattern_32[0] = 0x11111111;
        rx2_pattern_8[1] = 0xBB; rx2_pattern_16[1] = 0xBBBB; rx2_pattern_32[1] = 0x22222222;
        rx2_pattern_8[2] = 0xCC; rx2_pattern_16[2] = 0xCCCC; rx2_pattern_32[2] = 0x33333333;
        rx2_pattern_8[3] = 0xFF; rx2_pattern_16[3] = 0xFFFF; rx2_pattern_32[3] = 0xFFFFFFFF;
        rx2_pattern_8[4] = 0xFF; rx2_pattern_16[4] = 0xFFFF; rx2_pattern_32[4] = 0xFFFFFFFF;
    }
    /* Exception: master TX > master RX . */
    if (test_cases[tc_id].slave_tx_cnt > test_cases[tc_id].slave_rx_cnt) {
        rx2_pattern_8[0] = 0xAA; rx2_pattern_16[0] = 0xAAAA; rx2_pattern_32[0] = 0x11111111;
        rx2_pattern_8[1] = 0xBB; rx2_pattern_16[1] = 0xBBBB; rx2_pattern_32[1] = 0x22222222;
        rx2_pattern_8[2] = 0xCC; rx2_pattern_16[2] = 0xCCCC; rx2_pattern_32[2] = 0x33333333;
        rx2_pattern_8[3] = 0;    rx2_pattern_16[3] = 0;      rx2_pattern_32[3] = 0;
        rx2_pattern_8[4] = 0;    rx2_pattern_16[4] = 0;      rx2_pattern_32[4] = 0;
    }
    /* Exception: slave TX < slave RX . */
    if (test_cases[tc_id].slave_tx_cnt < test_cases[tc_id].slave_rx_cnt) {
        rx1_pattern_8[0] = 0x11; rx1_pattern_16[0] = 0x1111; rx1_pattern_32[0] = 0x11111111;
        rx1_pattern_8[1] = 0x22; rx1_pattern_16[1] = 0x2222; rx1_pattern_32[1] = 0x22222222;
        rx1_pattern_8[2] = 0x33; rx1_pattern_16[2] = 0x3333; rx1_pattern_32[2] = 0x33333333;
        rx1_pattern_8[3] = 0xFF; rx1_pattern_16[3] = 0xFFFF; rx1_pattern_32[3] = 0xFFFFFFFF;
        rx1_pattern_8[4] = 0xFF; rx1_pattern_16[4] = 0xFFFF; rx1_pattern_32[4] = 0xFFFFFFFF;

        rx2_pattern_8[0] = 0xAA; rx2_pattern_16[0] = 0xAAAA; rx2_pattern_32[0] = 0x11111111;
        rx2_pattern_8[1] = 0xBB; rx2_pattern_16[1] = 0xBBBB; rx2_pattern_32[1] = 0x22222222;
        rx2_pattern_8[2] = 0xCC; rx2_pattern_16[2] = 0xCCCC; rx2_pattern_32[2] = 0x33333333;
        rx2_pattern_8[3] = 0xFF; rx2_pattern_16[3] = 0xFFFF; rx2_pattern_32[3] = 0xFFFFFFFF;
        rx2_pattern_8[4] = 0xFF; rx2_pattern_16[4] = 0xFFFF; rx2_pattern_32[4] = 0xFFFFFFFF;
    }

    /* Exception: master TX buffer undefined . */
    if (!test_cases[tc_id].master_tx_defined) {
        rx2_pattern_8[0] = 0xFF; rx2_pattern_16[0] = 0xFFFF; rx2_pattern_32[0] = 0xFFFFFFFF;
        rx2_pattern_8[1] = 0xFF; rx2_pattern_16[1] = 0xFFFF; rx2_pattern_32[1] = 0xFFFFFFFF;
        rx2_pattern_8[2] = 0xFF; rx2_pattern_16[2] = 0xFFFF; rx2_pattern_32[2] = 0xFFFFFFFF;
        rx2_pattern_8[3] = 0xFF; rx2_pattern_16[3] = 0xFFFF; rx2_pattern_32[3] = 0xFFFFFFFF;
        rx2_pattern_8[4] = 0xFF; rx2_pattern_16[4] = 0xFFFF; rx2_pattern_32[4] = 0xFFFFFFFF;
    }

    /* Exception: slave TX buffer undefined . */
    if (!test_cases[tc_id].slave_tx_defined) {
        rx1_pattern_8[0] = 0xFF; rx1_pattern_16[0] = 0xFFFF; rx1_pattern_32[0] = 0xFFFFFFFF;
        rx1_pattern_8[1] = 0xFF; rx1_pattern_16[1] = 0xFFFF; rx1_pattern_32[1] = 0xFFFFFFFF;
        rx1_pattern_8[2] = 0xFF; rx1_pattern_16[2] = 0xFFFF; rx1_pattern_32[2] = 0xFFFFFFFF;
        rx1_pattern_8[3] = 0xFF; rx1_pattern_16[3] = 0xFFFF; rx1_pattern_32[3] = 0xFFFFFFFF;
        rx1_pattern_8[4] = 0xFF; rx1_pattern_16[4] = 0xFFFF; rx1_pattern_32[4] = 0xFFFFFFFF;

        rx2_pattern_8[0] = 0xFF; rx2_pattern_16[0] = 0xFFFF; rx2_pattern_32[0] = 0xFFFFFFFF;
        rx2_pattern_8[1] = 0xFF; rx2_pattern_16[1] = 0xFFFF; rx2_pattern_32[1] = 0xFFFFFFFF;
        rx2_pattern_8[2] = 0xFF; rx2_pattern_16[2] = 0xFFFF; rx2_pattern_32[2] = 0xFFFFFFFF;
        rx2_pattern_8[3] = 0xFF; rx2_pattern_16[3] = 0xFFFF; rx2_pattern_32[3] = 0xFFFFFFFF;
        rx2_pattern_8[4] = 0xFF; rx2_pattern_16[4] = 0xFFFF; rx2_pattern_32[4] = 0xFFFFFFFF;
    }

    /* Exception: slave RX buffer undefined . */
    if (!test_cases[tc_id].slave_rx_defined) {
        rx2_pattern_8[0] = 0x11; rx2_pattern_16[0] = 0x1111; rx2_pattern_32[0] = 0x11111111;
        rx2_pattern_8[1] = 0x22; rx2_pattern_16[1] = 0x2222; rx2_pattern_32[1] = 0x22222222;
        rx2_pattern_8[2] = 0x33; rx2_pattern_16[2] = 0x3333; rx2_pattern_32[2] = 0x33333333;
        rx2_pattern_8[3] = 0x44; rx2_pattern_16[3] = 0x4444; rx2_pattern_32[3] = 0x44444444;
        rx2_pattern_8[4] = 0x55; rx2_pattern_16[4] = 0x5555; rx2_pattern_32[4] = 0x55555555;
    }

    /* Init pointers to TX/RX buffers based on symbol size. */
    if (test_cases[tc_id].symbol_size <= 8) {

        /* Handle symbol size. */
        uint8_t sym_mask = ((1 << test_cases[tc_id].symbol_size) - 1);
        for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
            tx_pattern_8[i] = (tx_pattern_8[i] & sym_mask);
            rx1_pattern_8[i] = (rx1_pattern_8[i] & sym_mask);
            rx2_pattern_8[i] = (rx2_pattern_8[i] & sym_mask);
        }
        if (test_cases[tc_id].master_tx_defined) {
            p_tx_buf = tx_buff_uint8;
            p_tx_pattern = tx_pattern_8;
        }
        if (test_cases[tc_id].master_rx_defined) {
            p_rx_buf = rx_buff_uint8;
            p_rx1_pattern = rx1_pattern_8;
            p_rx2_pattern = rx2_pattern_8;
        }
        p_fill_sym = (void*) &fill_symbol_8;
        buff_size = 1 * TEST_SYM_CNT;
        memcpy(tx_buff_uint8, tx_pattern_8, sizeof(tx_buff_uint8));
        set_buffer(rx_buff_uint8, sizeof(rx_buff_uint8), 0x00);
    } else if (test_cases[tc_id].symbol_size <= 16) {
        /* Handle symbol size. */
        uint16_t sym_mask = ((1 << test_cases[tc_id].symbol_size) - 1);
        for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
            tx_pattern_16[i] = (tx_pattern_16[i] & sym_mask);
            rx1_pattern_16[i] = (rx1_pattern_16[i] & sym_mask);
            rx2_pattern_16[i] = (rx2_pattern_16[i] & sym_mask);
        }
        if (test_cases[tc_id].master_tx_defined) {
            p_tx_buf = tx_buff_uint16;
            p_tx_pattern = tx_pattern_16;
        }
        if (test_cases[tc_id].master_rx_defined) {
            p_rx_buf = rx_buff_uint16;
            p_rx1_pattern = rx1_pattern_16;
            p_rx2_pattern = rx2_pattern_16;
        }
        p_fill_sym = (void*) &fill_symbol_16;
        buff_size = 2 * TEST_SYM_CNT;
        memcpy(tx_buff_uint16, tx_pattern_16, sizeof(tx_buff_uint16));
        set_buffer(rx_buff_uint16, sizeof(rx_buff_uint16), 0x00);
    } else {
        /* Handle symbol size. */
        uint32_t sym_mask = ((1 << test_cases[tc_id].symbol_size) - 1);
        for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
            tx_pattern_32[i] = (tx_pattern_16[i] & sym_mask);
            rx1_pattern_32[i] = (rx1_pattern_16[i] & sym_mask);
            rx2_pattern_32[i] = (rx2_pattern_16[i] & sym_mask);
        }
        if (test_cases[tc_id].master_tx_defined) {
            p_tx_buf = tx_buff_uint32;
            p_tx_pattern = tx_pattern_32;
        }
        if (test_cases[tc_id].master_rx_defined) {
            p_rx_buf = rx_buff_uint32;
            p_rx1_pattern = rx1_pattern_32;
            p_rx2_pattern = rx2_pattern_32;
        }
        p_fill_sym = (void*) &fill_symbol_32;
        buff_size = 4 * TEST_SYM_CNT;
        memcpy(tx_buff_uint32, tx_pattern_32, sizeof(tx_buff_uint32));
        set_buffer(rx_buff_uint16, sizeof(rx_buff_uint16), 0x00);
    }
}

void handle_ss(DigitalOut * ss, bool select)
{
    if (ss) {
        if (select) {
            *ss = 0;
        } else {
            *ss = 1;
        }
    }
}

bool check_capabilities(spi_capabilities_t *p_cabs, uint32_t symbol_size, bool slave, bool half_duplex)
{
    if (!(p_cabs->word_length & (1 << (symbol_size - 1))) ||
        (slave && !p_cabs->support_slave_mode) ||
        (half_duplex && !p_cabs->half_duplex)) {
        return false;
    }

    return true;
}

void test_transfer_master()
{
    uint32_t count;
    int8_t status;
    spi_capabilities_t capabilities = { 0 };

    spi_get_capabilities(spi_get_module(SPI_MASTER_MOSI,
                                        SPI_MASTER_MISO,
                                        SPI_MASTER_CLK),
                         NC,
                         &capabilities);

    for (uint32_t tc_id = 0; tc_id < (sizeof(test_cases) / sizeof(config_test_case_t)); tc_id++) {
        DigitalOut *ss_pin = NULL;
        PinName ss = SPI_MASTER_SS;
        PinName miso = SPI_MASTER_MISO;
        PinName mosi = SPI_MASTER_MOSI;
        uint32_t clocked_symbols_1 = (test_cases[tc_id].slave_tx_cnt + test_cases[tc_id].slave_rx_cnt);
        uint32_t clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);

        printf("---> Test Case: [%lu] \n", tc_id);

        /* Adapt Full duplex/Half duplex settings. */
        switch(test_cases[tc_id].duplex) {
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
            ss_pin = new DigitalOut(SPI_MASTER_SS);
            ss = NC;
            *ss_pin = 1;
        }

        spi_init(&spi_master, false, mosi, miso, SPI_MASTER_CLK, ss);

        /* Continue if master can handle the config, otherwise skip it. */
        if (check_capabilities(&capabilities, test_cases[tc_id].symbol_size, false, test_cases[tc_id].duplex)) {

            /* Adapt min/max frequency for testing based of capabilities. */
            switch(test_cases[tc_id].freq_hz) {
                case FREQ_MIN:
                    test_cases[tc_id].freq_hz = capabilities.minimum_frequency;
                break;

                case FREQ_MAX:
                    test_cases[tc_id].freq_hz = capabilities.maximum_frequency;
                break;

                default:

                break;
            }

            spi_format(&spi_master,
                       test_cases[DEFAULT_CFG].symbol_size,
                       test_cases[DEFAULT_CFG].mode,
                       test_cases[DEFAULT_CFG].bit_ordering);

            spi_frequency(&spi_master, test_cases[DEFAULT_CFG].freq_hz);

            /* Send config to slave. Slave will return status if given config can be
             * handled by slave.
             */
            wait_until_button_pressed();
            handle_ss(ss_pin, true);
            count = spi_transfer(&spi_master, &test_cases[tc_id], CONFIG_LEN, NULL, 0, (void*) &fill_symbol_8);
            handle_ss(ss_pin, false);

            wait_until_button_pressed();
            handle_ss(ss_pin, true);
            count = spi_transfer(&spi_master, NULL, 0, &status, CONFIG_STATUS_LEN, (void*) &fill_symbol_8);
            handle_ss(ss_pin, false);

            /* Continue if slave can handle the config, otherwise skip it. */
            if (status == CONFIG_STATUS_OK) {
                spi_format(&spi_master,
                           test_cases[tc_id].symbol_size,
                           test_cases[tc_id].mode,
                           test_cases[tc_id].bit_ordering);

                spi_frequency(&spi_master, test_cases[tc_id].freq_hz);

                init_transmission_buffers(tc_id);

                wait_until_button_pressed();
                handle_ss(ss_pin, true);
                count = spi_transfer(&spi_master, p_tx_buf, test_cases[tc_id].master_tx_cnt, p_rx_buf, test_cases[tc_id].master_rx_cnt, (void*) p_fill_sym);
                handle_ss(ss_pin, false);

                TEST_ASSERT_EQUAL(true, check_buffers(p_rx1_pattern, p_rx_buf, buff_size));
                TEST_ASSERT_EQUAL(true, check_buffers(p_tx_pattern, p_tx_buf, buff_size));
                TEST_ASSERT_EQUAL(clocked_symbols_1, count);

                /* Check what slave received in the last transfer (send received data to slave
                 * and slave will do the same). */

                if (p_tx_buf && p_rx_buf) {
                    if (test_cases[tc_id].symbol_size <= 8) {
                        memcpy(p_tx_buf, p_rx_buf, sizeof(tx_buff_uint8));
                    } else if (test_cases[tc_id].symbol_size <= 16) {
                        memcpy(p_tx_buf, p_rx_buf, sizeof(tx_buff_uint16));
                    } else {
                        memcpy(p_tx_buf, p_rx_buf, sizeof(tx_buff_uint32));
                    }
                }

                set_buffer(p_rx_buf, buff_size, 0x00);

                wait_until_button_pressed();
                handle_ss(ss_pin, true);
                count = spi_transfer(&spi_master, p_tx_buf, TEST_SYM_CNT, p_rx_buf, TEST_SYM_CNT, (void*) p_fill_sym);
                handle_ss(ss_pin, false);

                TEST_ASSERT_EQUAL(true, check_buffers(p_rx2_pattern, p_rx_buf, buff_size));
                TEST_ASSERT_EQUAL(true, check_buffers(p_rx1_pattern, p_tx_buf, buff_size));
                TEST_ASSERT_EQUAL(clocked_symbols_2, count);

            } else if (status == CONFIG_STATUS_NOT_SUPPORTED) {
                printf("Config not supported by slave. Skipping. \n");
            } else {
                printf("status: 0x%X \n", status);
                TEST_ASSERT_TRUE_MESSAGE(false, "Invalid configuration status. Communication error!");
            }
        } else {
            printf("Format not supported by master. Skipping. \n");
        }

        spi_free(&spi_master);
    }
}

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(40, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}


Case cases[] = {
    Case("SPI - format and frequency testing (automatic ss)", test_transfer_master),
    //Case("SPI - single master transfer test", test_transfer_master_single, greentea_failure_handler)
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
