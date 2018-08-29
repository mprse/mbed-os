/*
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

// Waiting for public release to enable, currently there are problems with the SD driver

// check if SPI is supported on this device
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

#define TX_BUF_SIZE 10
#define RX_BUF_SIZE 10

static uint8_t tx_buff_uint8[TX_BUF_SIZE];
static uint8_t rx_buff_uint8[RX_BUF_SIZE];
static uint16_t tx_buff_uint16[TX_BUF_SIZE];
static uint16_t rx_buff_uint16[RX_BUF_SIZE];
static uint32_t tx_buff_uint32[TX_BUF_SIZE];
static uint32_t rx_buff_uint32[RX_BUF_SIZE];

static const uint8_t fill_symbol_8 = 0x55;
static const uint16_t fill_symbol_16 = 0x5555;
static const uint32_t fill_symbol_32 = 0x55555555;

/* There is no tolerance defined how much requested frequency can differ from actual frequency.
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

static PinName get_spi_pin(spi_pin_type_t pin_type, int peripheral)
{
    const PinMap * pin_maps[] = {
        PinMap_SPI_SCLK,
        PinMap_SPI_MOSI,
        PinMap_SPI_MISO,
        PinMap_SPI_SSEL };

    const PinMap * pin_map = pin_maps[pin_type];

    while (pin_map->peripheral != NC) {
        if (pin_map->peripheral == peripheral) {
            break;
        }

        pin_map++;
    }

    TEST_ASSERT_MESSAGE(pin_map->pin != NC, "No pin associated with the peripheral");

    return pin_map->pin;
}

typedef struct {
    uint8_t symbol_size;
    spi_mode_t mode;
    spi_bit_ordering_t bit_ordering;
    uint32_t freq_hz;
} transmit_test_case_t;


static transmit_test_case_t test_cases[] = {
        /* default config */
        {8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST, FREQ_1MHZ},
        /* word length testing */
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

void test_get_module()
{
    SPIName spi_name;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {
        spi_name = spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName)spi_id, spi_name);

        spi_name = spi_get_module(NC,
                                  get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName)spi_id, spi_name);

        spi_name = spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                  NC,
                                  get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id));

        TEST_ASSERT_EQUAL((SPIName)spi_id, spi_name);
    }
}

void test_get_capabilities()
{
    spi_capabilities_t capabilities;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        /* Pass SS pin to spi_get_capabilities. */
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id),
                             &capabilities);

        TEST_ASSERT(capabilities.minimum_frequency <= 200000);
        TEST_ASSERT(capabilities.maximum_frequency >= 2000000);
        TEST_ASSERT_TRUE(capabilities.word_length & CAPABILITY_WORD_LENGTH_8);

        /* Do not pass SS pin to spi_get_capabilities. */
        spi_get_capabilities(spi_get_module(get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                                            get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id)),
                             NC,
                             &capabilities);

        TEST_ASSERT(capabilities.minimum_frequency >= 200000);
        TEST_ASSERT(capabilities.maximum_frequency <= 2000000);
        TEST_ASSERT_TRUE(capabilities.word_length & CAPABILITY_WORD_LENGTH_8);
    }
}

void test_init_free()
{
    spi_t spi_obj = {0};
    spi_capabilities_t capabilities = {0};

    void spi_init(spi_t *obj, bool is_slave, PinName MISO, PinName MOSI, PinName MCLK, PinName SS);

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
                 NC
                 );

        //spi_free(&spi_obj);

        /* Half duplex: MASTER <--- SLAVE. */
        spi_init(&spi_obj,
                 false,
                 NC,
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC
                 );

        //spi_free(&spi_obj);

        /* Half duplex: MASTER ---> SLAVE. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 NC,
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC
                 );

        //spi_free(&spi_obj);

        if (capabilities.support_slave_mode) {
            /* SPI slave - SS pin provided. */
            /* Full duplex. */
            spi_init(&spi_obj,
                     true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id)
                     );

            //spi_free(&spi_obj);

            /* Half duplex: MASTER <--- SLAVE. */
            spi_init(&spi_obj,
                    true,
                     NC,
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id)
                     );

            //spi_free(&spi_obj);

            /* Half duplex: MASTER ---> SLAVE. */
            spi_init(&spi_obj,
                    true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     NC,
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id)
                     );

            //spi_free(&spi_obj);
        }
    }

    TEST_ASSERT_TRUE(true);
}

void test_set_format()
{
    spi_t spi_obj = {0};
    spi_capabilities_t capabilities = {0};

    void spi_init(spi_t *obj, bool is_slave, PinName MISO, PinName MOSI, PinName MCLK, PinName SS);

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
            /* Full duplex. */
            spi_init(&spi_obj,
                     true,
                     get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                     get_spi_pin(SPI_SSEL_PIN_TYPE, spi_id)
                     );

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

    TEST_ASSERT_TRUE(true);
}

void test_set_frequency()
{
    spi_t spi_obj = {0};
    uint32_t real_freq;

    for (uint32_t spi_id = 0; spi_id < SPI_COUNT; spi_id++) {

        /* SPI master - SS pin ignored. */
        spi_init(&spi_obj,
                 false,
                 get_spi_pin(SPI_MOSI_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_MISO_PIN_TYPE, spi_id),
                 get_spi_pin(SPI_SCLK_PIN_TYPE, spi_id),
                 NC
                 );

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

void test_transfer_master()
{
    spi_t spi_obj = {0};
    uint32_t real_freq;
    spi_capabilities_t capabilities = {0};

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

        for(uint32_t i = 0; i < (sizeof(test_cases)/sizeof(transmit_test_case_t)); i++) {

            if (capabilities.word_length & (1 << (test_cases[i].symbol_size - 1))) {

                spi_format(&spi_obj, test_cases[i].symbol_size, test_cases[i].mode, test_cases[i].bit_ordering);
                spi_frequency(&spi_obj, test_cases[i].freq_hz);

                if (test_cases[i].symbol_size < 8) {
                    p_tx_buf = tx_buff_uint8;
                    p_rx_buf = rx_buff_uint8;
                    p_fill_sym = (void*)&fill_symbol_8;
                } else if (test_cases[i].symbol_size < 16) {
                    p_tx_buf = tx_buff_uint16;
                    p_rx_buf = rx_buff_uint16;
                    p_fill_sym = (void*)&fill_symbol_16;
                } else {
                    p_tx_buf = tx_buff_uint32;
                    p_rx_buf = rx_buff_uint32;
                    p_fill_sym = (void*)&fill_symbol_32;
                }

                spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE, p_rx_buf, RX_BUF_SIZE, p_fill_sym);
            }
        }

        spi_free(&spi_obj);
    }
}

void test_transfer_master_fill_sym()
{
    spi_t spi_obj = {0};
    uint32_t real_freq;
    spi_capabilities_t capabilities = {0};

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

        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_frequency(&spi_obj, FREQ_1MHZ);

        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE - 5, p_rx_buf, RX_BUF_SIZE, p_fill_sym);
        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE, p_rx_buf, RX_BUF_SIZE - 5, p_fill_sym);
        spi_transfer(&spi_obj, NULL, TX_BUF_SIZE, p_rx_buf, RX_BUF_SIZE, p_fill_sym);
        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE - 5, NULL, RX_BUF_SIZE, p_fill_sym);

        spi_free(&spi_obj);
    }
}

void test_transfer_master_fill_sym()
{
    spi_t spi_obj = {0};
    uint32_t real_freq;
    spi_capabilities_t capabilities = {0};

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

        spi_format(&spi_obj, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);
        spi_frequency(&spi_obj, FREQ_1MHZ);

        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE - 5, p_rx_buf, RX_BUF_SIZE, p_fill_sym);
        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE, p_rx_buf, RX_BUF_SIZE - 5, p_fill_sym);
        spi_transfer(&spi_obj, NULL, TX_BUF_SIZE, p_rx_buf, RX_BUF_SIZE, p_fill_sym);
        spi_transfer(&spi_obj, p_tx_buf, TX_BUF_SIZE - 5, NULL, RX_BUF_SIZE, p_fill_sym);

        spi_free(&spi_obj);
    }
}

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(40, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Handle test failures, keep testing, dont stop
utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason)
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

// Handle test failures, stop on failure
utest::v1::status_t greentea_failure_handler_abort(const Case *const source, const failure_t reason)
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_ABORT;
}

// Test cases
 Case cases[] = {
     Case("SPI - get module name test", test_get_module, greentea_failure_handler),
     Case("SPI - get capabilities test", test_get_capabilities, greentea_failure_handler),
     Case("SPI - init, free", test_init_free, greentea_failure_handler),
     Case("SPI - set format", test_set_format, greentea_failure_handler),
     Case("SPI - set frequency", test_set_frequency, greentea_failure_handler),
     Case("SPI - master transfer", test_transfer_master, greentea_failure_handler),
     Case("SPI - fill symbol", test_transfer_master_fill_sym, greentea_failure_handler),
 };

 Specification specification(test_setup, cases);

// // Entry point into the tests
int main() {
    return !Harness::run(specification);
}
