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
    duplex_t duplex;
} config_test_case_t;

#define CONFIG_LEN (sizeof(config_test_case_t))
#define CONFIG_STATUS_LEN 1

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

    count = spi_transfer(obj, p_tx_buff, config->slave_tx_cnt, p_rx_buff, config->slave_rx_cnt, &fill_symbol);

    TEST_ASSERT_EQUAL_MESSAGE(clocked_symbols_1, count, "Transmission 1: Invalid number of clocked symbols.\r\n");

    /* Send data received from master in the previous transmission. */
    if (p_tx_buff && p_rx_buff) {
        memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
    }

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    count = spi_transfer(&spi_slave, p_tx_buff, TEST_SYM_CNT, p_rx_buff, TEST_SYM_CNT, &fill_symbol);

    TEST_ASSERT_EQUAL_MESSAGE(clocked_symbols_2, count, "Transmission 2: Invalid number of clocked symbols.\r\n");
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

int main()
{
    uint8_t fill_symbol = 0xFF;
    int8_t status;
    spi_capabilities_t capabilities = { 0 };

    printf(" ------- Welcome -------- \r\n");

    spi_get_capabilities(spi_get_module(SPI_SLAVE_MOSI,
                                        SPI_SLAVE_MISO,
                                        SPI_SLAVE_CLK),
                         SPI_SLAVE_SS,
                         &capabilities);

    while (true) {
        spi_init(&spi_slave, true, SPI_SLAVE_MOSI, SPI_SLAVE_MISO, SPI_SLAVE_CLK, SPI_SLAVE_SS);

        spi_format(&spi_slave, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

        config_test_case_t config = {0};

        spi_transfer(&spi_slave, NULL, 0, &config, CONFIG_LEN, &fill_symbol);

        /*
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

        */
        if (check_capabilities(&capabilities, config.symbol_size, true, config.duplex)) {
            status = 0x00;
            PinName miso = SPI_SLAVE_MISO;
            PinName mosi = SPI_SLAVE_MOSI;

            spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, NULL, 0, (void*) &fill_symbol);

            spi_free(&spi_slave);

            /* Adapt Full duplex/Half duplex settings. */
            switch(config.duplex) {
                case HALF_DUPLEX_MOSI:
                    miso = NC;
                break;

                case HALF_DUPLEX_MISO:
                    mosi = NC;
                break;

                default:

                break;
            }

            spi_init(&spi_slave, true, mosi, miso, SPI_SLAVE_CLK, SPI_SLAVE_SS);

            spi_format(&spi_slave, config.symbol_size, (spi_mode_t)config.mode, (spi_bit_ordering_t)config.bit_ordering);

            if (config.symbol_size <= 8) {
                slave_transfer<uint8_t>(&spi_slave, &config);
            } else if (config.symbol_size <= 16) {
                slave_transfer<uint16_t>(&spi_slave, &config);
            } else {
                slave_transfer<uint16_t>(&spi_slave, &config);
            }
        } else {
            status = 0x01;
            spi_transfer(&spi_slave, &status, CONFIG_STATUS_LEN, NULL, 0, &fill_symbol);
        }

        spi_free(&spi_slave);
    }
}

