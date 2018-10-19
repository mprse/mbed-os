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

#ifndef IS_MASTER
    #error [NOT_SUPPORTED] This test can not be run on CI. It is designed for manual testing.
#endif

/***** test configuration *****/

/* Only MASTER can initialise the transfer, so SLAVE needs to be ready
 * when MASTER starts clocking.
 * Specify the delay on master side before each transfer.
 * If this is not enough for your case please adapt the delay.
 * Alternatively you can specify the delay 0. In this case test will hang before
 * each transmission until the specified button is pressed.
 */
/* Pins configuration. */
#if IS_MASTER
#define SPI_MISO MBED_CONF_APP_SPI_MASTER_MISO
#define SPI_MOSI MBED_CONF_APP_SPI_MASTER_MOSI
#define SPI_CLK MBED_CONF_APP_SPI_MASTER_CLK
#define SPI_SS MBED_CONF_APP_SPI_MASTER_CS
#define TRANSMISSION_DELAY_MS MBED_CONF_APP_SPI_MASTER_DELAY
#define TRANSMISSION_BUTTON MBED_CONF_APP_SPI_MASTER_TRANSMISSION_START_BTN
#define TRANSMISSION_LED MBED_CONF_APP_SPI_MASTER_TRANSMISSION_START_LED
#define DEBUG MBED_CONF_APP_SPI_MASTER_DEBUG
#else
#define SPI_MISO MBED_CONF_APP_SPI_SLAVE_MISO
#define SPI_MOSI MBED_CONF_APP_SPI_SLAVE_MOSI
#define SPI_CLK MBED_CONF_APP_SPI_SLAVE_CLK
#define SPI_SS MBED_CONF_APP_SPI_SLAVE_CS
#define TRANSMISSION_DELAY_MS MBED_CONF_APP_SPI_SLAVE_DELAY
#define TRANSMISSION_BUTTON MBED_CONF_APP_SPI_SLAVE_TRANSMISSION_START_BTN
#define TRANSMISSION_LED MBED_CONF_APP_SPI_SLAVE_TRANSMISSION_START_LED
#define DEBUG MBED_CONF_APP_SPI_SLAVE_DEBUG
#endif

/******************************/

#define TEST_SYM_CNT 5

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)
#define FREQ_MIN    (0)
#define FREQ_MAX    (0xFFFFFFFF)

#define DEFAULT_CFG 0
#define CONFIG_STATUS_OK 0x55
#define CONFIG_STATUS_NOT_SUPPORTED 0xAA

typedef enum
{
    FULL_DUPLEX, HALF_DUPLEX_MOSI, HALF_DUPLEX_MISO
} duplex_t;

/* SPI test configuration. */
typedef struct
{
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
    bool sync;
} config_test_case_t;

#define CONFIG_LEN (sizeof(config_test_case_t))
#define CONFIG_STATUS_LEN 2

/* Function inits specified buffer with given pattern. */
static void set_buffer(void * addr, uint32_t size, char val)
{
    if (addr == NULL) {
        return;
    }

    char *p_char = (char*) addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = val;
    }
}

/* Function returns true if configuration is consistent with the capabilities of
 * the SPI peripheral, false otherwise. */
static bool check_capabilities(spi_capabilities_t *p_cabs, uint32_t symbol_size, bool slave, bool half_duplex)
{
    if (!(p_cabs->word_length & (1 << (symbol_size - 1))) ||
        (slave && !p_cabs->support_slave_mode) ||
        (half_duplex && !p_cabs->half_duplex))
#ifndef DEVICE_SPI_ASYNCH
        || (!sync_mode)
#endif
    {
        return false;
    }

    return true;
}

#ifdef DEVICE_SPI_ASYNCH
/* Callback function for SPI async transfers. */
static uint32_t context;
static spi_t *expected_obj;
void spi_async_callback(spi_t *obj, void *ctx, spi_async_event_t *event) {
    TEST_ASSERT_EQUAL(expected_obj, obj);
    *((uint32_t*)ctx) = event->transfered;
}
#endif

/* Function used to perform transmission using sync or async modes. */
static uint32_t sync_async_transfer(spi_t *obj, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len, const void *fill_symbol, bool sync_mode)
{
    uint32_t count = 0;

    if (sync_mode) {
        count = spi_transfer(obj, tx, tx_len, rx, rx_len, fill_symbol);
    }
#ifdef DEVICE_SPI_ASYNCH
    else {
        context = 0;
        expected_obj = obj;
        bool ret = spi_transfer_async(obj, tx, tx_len, rx, rx_len, fill_symbol, spi_async_callback, &context, DMA_USAGE_NEVER);
        TEST_ASSERT_EQUAL(true, ret);
        /* Wait here for the end of transmission. Callback will set context to the number of
         * transfered symbols. */
        while(!context);

        count = context;
    }
#endif

    return count;
}

/* Function waits before the transmission is triggered on the master side. */
static void wait_before_transmission()
{
    DigitalOut *tr_led = NULL;

    if(TRANSMISSION_LED != NC) {
        tr_led = new DigitalOut(TRANSMISSION_LED);
        *tr_led = 0;
    }

    if (TRANSMISSION_DELAY_MS) {
        wait_ms(TRANSMISSION_DELAY_MS);
    } else {
        DigitalIn button(TRANSMISSION_BUTTON);

        while (button.read() == 1);
        while (button.read() == 0);
    }

    if (tr_led) {
        *tr_led = 1;
    }
}

template<typename T>
static void dump_buffers(T *tx_pattern, T *rx1_pattern, T *rx2_pattern, T *tx_buff, T *rx_buff)
{
#if DEBUG
    printf("Dump buffers: \n");
    printf("tx_pattern : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", tx_pattern[0], tx_pattern[1], tx_pattern[2], tx_pattern[3], tx_pattern[4]);
    printf("rx1_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx1_pattern[0], rx1_pattern[1], rx1_pattern[2], rx1_pattern[3], rx1_pattern[4]);
    printf("rx2_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx2_pattern[0], rx2_pattern[1], rx2_pattern[2], rx2_pattern[3], rx2_pattern[4]);
    printf("tx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", tx_buff[0], tx_buff[1], tx_buff[2], tx_buff[3], tx_buff[4]);
    printf("rx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3], rx_buff[4]);
#endif
}

/* Debug function to print received configuration details. */
void dump_config(config_test_case_t *config)
{
#if DEBUG
    printf("TEST CASE CONFIGURATION\r\n");
    printf("symbol_size: %lu\r\n", (uint32_t) config->symbol_size);
    printf("spi_mode: %lu\r\n", (uint32_t) config->mode);
    printf("bit_ordering: %lu\r\n", (uint32_t) config->bit_ordering);
    printf("freq: %lu\r\n", (uint32_t) config->freq_hz);
    printf("master tx cnt: %lu\r\n", (uint32_t) config->master_tx_cnt);
    printf("master rx cnt: %lu\r\n", (uint32_t) config->master_rx_cnt);
    printf("slave tx cnt: %lu\r\n", (uint32_t) config->slave_tx_cnt);
    printf("slave rx cnt: %lu\r\n", (uint32_t) config->slave_rx_cnt);
    printf("master tx defined: %lu\r\n", (uint32_t) config->master_tx_defined);
    printf("master rx defined: %lu\r\n", (uint32_t) config->master_rx_defined);
    printf("slave tx defined: %lu\r\n", (uint32_t) config->slave_tx_defined);
    printf("slave rx defined: %lu\r\n", (uint32_t) config->slave_rx_defined);
    printf("auto ss: %lu\r\n", (uint32_t) config->auto_ss);
    printf("full duplex: %lu\r\n", (uint32_t) config->duplex);
    printf("sync mode: %lu\r\n", (uint32_t) config->sync);
    printf("---\r\n");
#endif
}
