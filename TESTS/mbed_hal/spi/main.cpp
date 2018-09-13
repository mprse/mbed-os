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

extern const PinMap PinMap_SPI_MOSI[];
extern const PinMap PinMap_SPI_MISO[];
extern const PinMap PinMap_SPI_SCLK[];
extern const PinMap PinMap_SPI_SSEL[];

#define CAPABILITY_WORD_LENGTH_8 (1<<7)

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)

#define BUF_SIZE 50

static uint8_t tx_buff_uint8[BUF_SIZE];
static uint8_t rx_buff_uint8[BUF_SIZE];
static uint16_t tx_buff_uint16[BUF_SIZE];
static uint16_t rx_buff_uint16[BUF_SIZE];
static uint32_t tx_buff_uint32[BUF_SIZE];
static uint32_t rx_buff_uint32[BUF_SIZE];

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

typedef enum {
    SPI_SCLK_PIN_TYPE,
    SPI_MOSI_PIN_TYPE,
    SPI_MISO_PIN_TYPE,
    SPI_SSEL_PIN_TYPE
} spi_pin_type_t;

/* Auxiliary function which return pin name associated with specified function and peripheral. */
static PinName get_spi_pin(spi_pin_type_t pin_function, int peripheral)
{
    const PinMap * pin_maps[] =
    { PinMap_SPI_SCLK, PinMap_SPI_MOSI, PinMap_SPI_MISO, PinMap_SPI_SSEL };

    const PinMap * pin_map = pin_maps[pin_function];

    while (pin_map->peripheral != NC) {
        if (pin_map->peripheral == peripheral) {
            break;
        }

        pin_map++;
    }

    TEST_ASSERT_MESSAGE(pin_map->pin != NC, "No pin associated with the peripheral");

    return pin_map->pin;
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

#ifdef DEVICE_SPI_ASYNCH

static spi_t *expected_spi_obj;
static uint32_t some_ctx;
static spi_async_event_t expected_event;
static volatile bool handler_called;

static void async_handler(spi_t *obj, void *ctx, spi_async_event_t *event)
{
    TEST_ASSERT_EQUAL(expected_spi_obj, obj);
    TEST_ASSERT_EQUAL((void*)&some_ctx, ctx);
    TEST_ASSERT_EQUAL(expected_event.transfered, event->transfered); // TBD: undefined at the moment

    handler_called = true;
}

#endif

/* Test that spi_get_module() returns the SPIName - unique identifier to the peripheral associated
 * to this SPI channel. */
void test_get_module()
{
    SPIName spi_name;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {
        spi_name = spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName )spi_id, spi_name);

        spi_name = spi_get_module(NC,
                                  get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName )spi_id, spi_name);

        spi_name = spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                  NC,
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName )spi_id, spi_name);
    }
}

/* Test that spi_get_capabilities() fills the given spi_capabilities_t instance with capabilities
 * of the specified SPI peripheral. */
void test_get_capabilities()
{
    spi_capabilities_t capabilities;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        /* SS pin passed to spi_get_capabilities. */
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        TEST_ASSERT(capabilities.minimum_frequency <= FREQ_200KHZ);
        TEST_ASSERT(capabilities.maximum_frequency >= FREQ_2MHZ);
        TEST_ASSERT_TRUE(capabilities.symbol_length & CAPABILITY_WORD_LENGTH_8);

        /* SS pin not passed to spi_get_capabilities. */
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             NC,
                             &capabilities);

        TEST_ASSERT(capabilities.minimum_frequency >= FREQ_200KHZ);
        TEST_ASSERT(capabilities.maximum_frequency <= FREQ_2MHZ);
        TEST_ASSERT_TRUE(capabilities.symbol_length & CAPABILITY_WORD_LENGTH_8);
    }
}

/* Test that spi_init() successfully initializes the pins and spi_free() can successfully
 * reset the pins to their default state. */
void test_init_free()
{
    spi_t spi_obj =
    { 0 };
    spi_capabilities_t capabilities =
    { 0 };

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);
        /* SPI master - SS pin ignored. */
        /* Full duplex. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        spi_free(&spi_obj);

        /* Half duplex: MASTER <--- SLAVE. */
        spi_init(&spi_obj,
                 false,
                 NC,
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        spi_free(&spi_obj);

        /* Half duplex: MASTER ---> SLAVE. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 NC,
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        spi_free(&spi_obj);

        if (capabilities.support_slave_mode) {
            /* SPI slave - SS pin provided. */
            /* Full duplex. */
            spi_init(&spi_obj,
                     true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id));

            spi_free(&spi_obj);

            /* Half duplex: MASTER <--- SLAVE. */
            spi_init(&spi_obj,
                     true,
                     NC,
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id));

            spi_free(&spi_obj);

            /* Half duplex: MASTER ---> SLAVE. */
            spi_init(&spi_obj,
                     true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     NC,
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id));

            spi_free(&spi_obj);
        }
    }

    /* Indicate that test case has been successfully executed. */
    TEST_ASSERT_TRUE(true);
}

/* Test that spi_format() sets/updates the transmission format of the SPI peripheral. */
void test_set_format()
{
    spi_t spi_obj =
    { 0 };
    spi_capabilities_t capabilities =
    { 0 };

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id), &capabilities);
        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        /* 8 bit word length is obligate, all possible combinations of SPI mode and bit ordering are obligate. */
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
        spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_LSB_FIRST);

        spi_free(&spi_obj);

        if (capabilities.support_slave_mode) {
            /* SPI slave - SS pin provided. */
            spi_init(&spi_obj,
                     true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id));

            /* 8 bit word length is obligate, all possible combinations of SPI mode and bit ordering are obligate. */
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_LSB_FIRST);
            spi_format(&spi_obj, 8, SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, SPI_BIT_ORDERING_LSB_FIRST);

            spi_free(&spi_obj);
        }
    }

    /* Indicate that test case has been successfully executed. */
    TEST_ASSERT_TRUE(true);
}

/* Test that test_set_frequency() sets the frequency used during the SPI transfer. */
void test_set_frequency()
{
    spi_t spi_obj =
    { 0 };
    uint32_t real_freq;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

        real_freq = spi_frequency(&spi_obj, FREQ_200KHZ);

        TEST_ASSERT_UINT32_WITHIN(FREQ_TOLERANCE(FREQ_200KHZ), FREQ_200KHZ, real_freq);

        real_freq = spi_frequency(&spi_obj, FREQ_1MHZ);

        TEST_ASSERT_UINT32_WITHIN(FREQ_TOLERANCE(FREQ_1MHZ), FREQ_1MHZ, real_freq);

        real_freq = spi_frequency(&spi_obj, FREQ_2MHZ);

        TEST_ASSERT_UINT32_WITHIN(FREQ_TOLERANCE(FREQ_2MHZ), FREQ_2MHZ, real_freq);

        spi_free(&spi_obj);
    }
}

/* Test that spi_transfer() can successfully perform transfer in master mode
 *  (TX/RX buffers are defined and have the same sizes) and returns the number of
 *  symbols clocked on the bus during this transfer. */
void test_transfer_master()
{
    spi_t spi_obj =
    { 0 };
    spi_capabilities_t capabilities =
    { 0 };

    void *p_tx_buf;
    void *p_rx_buf;
    void *p_fill_sym;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        for (uint32_t tc_id = 0; tc_id < (sizeof(test_cases) / sizeof(config_test_case_t)); tc_id++) {

            if (capabilities.symbol_length & (1 << (test_cases[tc_id].symbol_size - 1))) {

                spi_format(&spi_obj, test_cases[tc_id].symbol_size, test_cases[tc_id].mode,
                        test_cases[tc_id].bit_ordering);
                spi_frequency(&spi_obj, test_cases[tc_id].freq_hz);

                if (test_cases[tc_id].symbol_size < 8) {
                    p_tx_buf = tx_buff_uint8;
                    p_rx_buf = rx_buff_uint8;
                    p_fill_sym = (void*) &fill_symbol_8;
                } else if (test_cases[tc_id].symbol_size < 16) {
                    p_tx_buf = tx_buff_uint16;
                    p_rx_buf = rx_buff_uint16;
                    p_fill_sym = (void*) &fill_symbol_16;
                } else {
                    p_tx_buf = tx_buff_uint32;
                    p_rx_buf = rx_buff_uint32;
                    p_fill_sym = (void*) &fill_symbol_32;
                }

                const uint32_t sym_cnt = spi_transfer(&spi_obj, p_tx_buf, BUF_SIZE, p_rx_buf, BUF_SIZE, p_fill_sym);

                TEST_ASSERT_EQUAL(BUF_SIZE, sym_cnt);
            }
        }

        spi_free(&spi_obj);
    }
}

/* Test that spi_transfer() can successfully perform transfer in master mode
 * (TX/RX buffers are undefined or have different sizes) and returns the number of
 * symbols clocked on the bus during this transfer. */
void test_transfer_master_fill_sym()
{
    spi_t spi_obj =
    { 0 };
    spi_capabilities_t capabilities =
    { 0 };
    uint32_t sym_cnt;

    void *p_tx_buf;
    void *p_rx_buf;
    void *p_fill_sym;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC);

        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_frequency(&spi_obj, FREQ_1MHZ);

        sym_cnt = spi_transfer(&spi_obj, p_tx_buf, BUF_SIZE / 2, p_rx_buf, BUF_SIZE, p_fill_sym);
        TEST_ASSERT_EQUAL(BUF_SIZE, sym_cnt);

        sym_cnt = spi_transfer(&spi_obj, p_tx_buf, BUF_SIZE, p_rx_buf, BUF_SIZE / 2, p_fill_sym);
        TEST_ASSERT_EQUAL(BUF_SIZE, sym_cnt);

        sym_cnt = spi_transfer(&spi_obj, NULL, BUF_SIZE, p_rx_buf, BUF_SIZE, p_fill_sym);
        TEST_ASSERT_EQUAL(BUF_SIZE, sym_cnt);

        sym_cnt = spi_transfer(&spi_obj, p_tx_buf, BUF_SIZE, NULL, BUF_SIZE, p_fill_sym);
        TEST_ASSERT_EQUAL(BUF_SIZE, sym_cnt);

        spi_free(&spi_obj);
    }
}

#ifdef DEVICE_SPI_ASYNCH
/* Test that spi_transfer_async() can successfully perform asynchronous transfer
 * in master mode. */
void test_transfer_master_async()
{
    spi_t spi_obj = {0};
    spi_capabilities_t capabilities = {0};

    const DMAUsage dma_modes[] = {
        DMA_USAGE_NEVER,
        DMA_USAGE_OPPORTUNISTIC,
        DMA_USAGE_ALWAYS,
        DMA_USAGE_TEMPORARY_ALLOCATED,
        DMA_USAGE_ALLOCATED
    };

    void *p_tx_buf;
    void *p_rx_buf;
    void *p_fill_sym;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC
        );

        for(uint32_t tc_id = 0; tc_id < (sizeof(test_cases)/sizeof(config_test_case_t)); tc_id++) {

            if (capabilities.symbol_length & (1 << (test_cases[tc_id].symbol_size - 1))) {

                spi_format(&spi_obj, test_cases[tc_id].symbol_size, test_cases[tc_id].mode, test_cases[tc_id].bit_ordering);
                spi_frequency(&spi_obj, test_cases[tc_id].freq_hz);

                if (test_cases[tc_id].symbol_size < 8) {
                    p_tx_buf = tx_buff_uint8;
                    p_rx_buf = rx_buff_uint8;
                    p_fill_sym = (void*)&fill_symbol_8;
                } else if (test_cases[tc_id].symbol_size < 16) {
                    p_tx_buf = tx_buff_uint16;
                    p_rx_buf = rx_buff_uint16;
                    p_fill_sym = (void*)&fill_symbol_16;
                } else {
                    p_tx_buf = tx_buff_uint32;
                    p_rx_buf = rx_buff_uint32;
                    p_fill_sym = (void*)&fill_symbol_32;
                }

                for (uint32_t mode_id = 0; mode_id < (sizeof(dma_modes) / sizeof(DMAUsage)); mode_id++) {
                    expected_spi_obj = &spi_obj;
                    handler_called = false;
                    expected_event.transfered = 0; // Success will have to be indicated here

                    const bool async_status = spi_transfer_async(&spi_obj, p_tx_buf, BUF_SIZE, p_rx_buf, BUF_SIZE, p_fill_sym, (void*)&some_ctx, dma_modes[mode_id]);

                    TEST_ASSERT_EQUAL(true, async_status);

                    while (!handler_called) {
                        /* Wait until the end of the spi transmission. */
                    }

                    TEST_ASSERT_EQUAL(true, handler_called);
                }
            }
        }
        spi_free(&spi_obj);
    }
}

/* Test that spi_transfer_async_abort() can successfully abort an on-going async transfer. */
void test_transfer_master_async_abort()
{
    spi_t spi_obj = {0};
    spi_capabilities_t capabilities = {0};

    const DMAUsage dma_modes[] = {
        DMA_USAGE_NEVER,
        DMA_USAGE_OPPORTUNISTIC,
        DMA_USAGE_ALWAYS,
        DMA_USAGE_TEMPORARY_ALLOCATED,
        DMA_USAGE_ALLOCATED
    };

    void *p_tx_buf;
    void *p_rx_buf;
    void *p_fill_sym;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                false,
                get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                NC
        );

        for(uint32_t tc_id = 0; tc_id < (sizeof(test_cases)/sizeof(config_test_case_t)); tc_id++) {

            if (capabilities.symbol_length & (1 << (test_cases[tc_id].symbol_size - 1))) {

                spi_format(&spi_obj, test_cases[tc_id].symbol_size, test_cases[tc_id].mode, test_cases[tc_id].bit_ordering);
                spi_frequency(&spi_obj, test_cases[tc_id].freq_hz);

                if (test_cases[tc_id].symbol_size < 8) {
                    p_tx_buf = tx_buff_uint8;
                    p_rx_buf = rx_buff_uint8;
                    p_fill_sym = (void*)&fill_symbol_8;
                } else if (test_cases[tc_id].symbol_size < 16) {
                    p_tx_buf = tx_buff_uint16;
                    p_rx_buf = rx_buff_uint16;
                    p_fill_sym = (void*)&fill_symbol_16;
                } else {
                    p_tx_buf = tx_buff_uint32;
                    p_rx_buf = rx_buff_uint32;
                    p_fill_sym = (void*)&fill_symbol_32;
                }

                for (uint32_t mode_id = 0; mode_id < (sizeof(dma_modes) / sizeof(DMAUsage)); mode_id++) {
                    expected_spi_obj = &spi_obj;
                    expected_event.transfered = 0; // Abort will have to be indicated here
                    handler_called = false;

                    const bool async_status = spi_transfer_async(&spi_obj, p_tx_buf, BUF_SIZE, p_rx_buf, BUF_SIZE, p_fill_sym, (void*)&some_ctx, dma_modes[mode_id]);

                    TEST_ASSERT_EQUAL(true, async_status);

                    /* Transfer of 50 bytes @ 1 MHz should take about 50 us.
                     * This should be enough time to call abort during the transfer.
                     */
                    spi_transfer_async_abort(spi_obj);

                    while (!handler_called) {
                        /* Wait until the end of the spi transmission. */
                    }

                    TEST_ASSERT_EQUAL(true, handler_called);
                }
            }
        }
        spi_free(&spi_obj);
    }
}
#endif

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
    Case("SPI - get module name test", test_get_module, greentea_failure_handler),
    Case("SPI - get capabilities test", test_get_capabilities, greentea_failure_handler),
    Case("SPI - init, free test", test_init_free, greentea_failure_handler),
    Case("SPI - set format test", test_set_format, greentea_failure_handler),
    Case("SPI - set frequency test", test_set_frequency, greentea_failure_handler),
    Case("SPI - master transfer test", test_transfer_master, greentea_failure_handler),
    Case("SPI - fill symbol test", test_transfer_master_fill_sym, greentea_failure_handler),
#ifdef DEVICE_SPI_ASYNCH
    Case("SPI - master transfer async test", test_transfer_master_async, greentea_failure_handler),
    Case("SPI - master transfer async abort test", test_transfer_master_async_abort, greentea_failure_handler),
#endif
    };

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}

