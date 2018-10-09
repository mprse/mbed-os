/*
 * Copyright (c) 2018 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed_config.h"

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)
#define FREQ_MIN    (0)
#define FREQ_MAX    (0xFFFFFFFF)

#define TEST_SYM_CNT 5

#define TRANSMISSION_DELAY_MS 100
#define TRANSMISSION_BUTTON NC

DigitalOut led(LED2);

#define DEBUG MBED_CONF_APP_SPI_SLAVE_DEBUG

#define CMDLINE_RETCODE_TEST_NOT_SUPPORTED      -100
#define CMDLINE_RETCODE_TEST_FAILED             -101

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

template<typename T>
static void dump_buffers(T *tx_pattern, T *rx1_pattern, T *rx2_pattern, T *tx_buff, T *rx_buff)
{
#if DEBUG
    typedef unsigned int u32;
    printf("Slave - buffers dump: \r\n");
    printf("tx_pattern : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", (u32) tx_pattern[0], (u32) tx_pattern[1], (u32) tx_pattern[2], (u32) tx_pattern[3], (u32) tx_pattern[4]);
    printf("rx1_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", (u32) rx1_pattern[0], (u32) rx1_pattern[1], (u32) rx1_pattern[2], (u32) rx1_pattern[3], (u32) rx1_pattern[4]);
    printf("rx2_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", (u32) rx2_pattern[0], (u32) rx2_pattern[1], (u32) rx2_pattern[2], (u32) rx2_pattern[3], (u32) rx2_pattern[4]);
    printf("tx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", (u32) tx_buff[0], (u32) tx_buff[1], (u32) tx_buff[2], (u32) tx_buff[3], (u32) tx_buff[4]);
    printf("rx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", (u32) rx_buff[0], (u32) rx_buff[1], (u32) rx_buff[2], (u32) rx_buff[3], (u32) rx_buff[4]);
#endif
}

/* Debug function to print received configuration details. */
void dump_config(config_test_case_t *config)
{
#if DEBUG
    printf("TEST CASE CONFIGURATION\r\n");
    printf("symbol_size:       %lu\r\n", (uint32_t) config->symbol_size);
    printf("spi_mode:          %lu\r\n", (uint32_t) config->mode);
    printf("bit_ordering:      %lu\r\n", (uint32_t) config->bit_ordering);
    printf("freq:              %lu\r\n", (uint32_t) config->freq_hz);
    printf("master tx cnt:     %lu\r\n", (uint32_t) config->master_tx_cnt);
    printf("master rx cnt:     %lu\r\n", (uint32_t) config->master_rx_cnt);
    printf("slave tx cnt:      %lu\r\n", (uint32_t) config->slave_tx_cnt);
    printf("slave rx cnt:      %lu\r\n", (uint32_t) config->slave_rx_cnt);
    printf("master tx defined: %lu\r\n", (uint32_t) config->master_tx_defined);
    printf("master rx defined: %lu\r\n", (uint32_t) config->master_rx_defined);
    printf("slave tx defined:  %lu\r\n", (uint32_t) config->slave_tx_defined);
    printf("slave rx defined:  %lu\r\n", (uint32_t) config->slave_rx_defined);
    printf("auto ss:           %lu\r\n", (uint32_t) config->auto_ss);
    printf("full duplex:       %lu\r\n", (uint32_t) config->duplex);
#endif
}

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
            return false;
        }
        p_byte_pattern++;
        p_byte_buffer++;
        size--;
    }

    return true;
}

/* Function waits before the transmission. */
static void wait_before_transmission()
{
    if (TRANSMISSION_DELAY_MS) {
        wait_ms(TRANSMISSION_DELAY_MS);
    } else {
        DigitalIn button(TRANSMISSION_BUTTON);

        while (button.read() == 1);
        while (button.read() == 0);
    }
}

#ifdef DEVICE_SPI_ASYNCH
/* Callback function for SPI async transfers. */
static uint32_t context;
void spi_async_callback(spi_t *obj, void *ctx, spi_async_event_t *event) {
    *((uint32_t*)ctx) = event->transfered;
}
#endif

/* Function returns true if configuration is consistent with the capabilities of
 * the SPI peripheral, false otherwise. */
static int check_capabilities(uint32_t symbol_size, bool slave, bool half_duplex, bool sync_mode)
{
    spi_capabilities_t capabilities = { 0 };
    spi_get_capabilities(spi_get_module(MBED_CONF_APP_SPI_SLAVE_MOSI,
                                        MBED_CONF_APP_SPI_SLAVE_MISO,
                                        MBED_CONF_APP_SPI_SLAVE_CLK),
                         MBED_CONF_APP_SPI_SLAVE_CS,
                         &capabilities);

    if (!(capabilities.word_length & (1 << (symbol_size - 1))) ||
         (slave && !capabilities.support_slave_mode) ||
         (half_duplex && !capabilities.half_duplex)
#ifndef DEVICE_SPI_ASYNCH
            || (!sync_mode)
#endif
            ) {
        printf("SKIP: Configuration not supported by master.\r\n");
    }

    return CMDLINE_RETCODE_SUCCESS;
}

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
        bool ret = spi_transfer_async(obj, tx, tx_len, rx, rx_len, fill_symbol, spi_async_callback, &context, DMA_USAGE_NEVER);
        /* Wait here for the end of transmission. Callback will set context to the number of
         * transfered symbols. */
        while(!context);

        count = context;
    }
#endif

    return count;
}

/* Function initialises RX, TX buffers before transmission. */
template<typename T>
static void init_transmission_buffers(config_test_case_t *config, T *p_tx_pattern, T *p_rx1_pattern, T *p_rx2_pattern, T *p_tx_buff, T *p_rx_buff, T *p_fill_symbol)
{
    /* Default patterns for TX/RX buffers. */
    set_buffer(&p_tx_pattern[0], sizeof(T), 0x11);
    set_buffer(&p_tx_pattern[1], sizeof(T), 0x22);
    set_buffer(&p_tx_pattern[2], sizeof(T), 0x33);
    set_buffer(&p_tx_pattern[3], sizeof(T), 0x44);
    set_buffer(&p_tx_pattern[4], sizeof(T), 0x55);

    set_buffer(&p_rx1_pattern[0], sizeof(T), 0xAA);
    set_buffer(&p_rx1_pattern[1], sizeof(T), 0xBB);
    set_buffer(&p_rx1_pattern[2], sizeof(T), 0xCC);
    set_buffer(&p_rx1_pattern[3], sizeof(T), 0xDD);
    set_buffer(&p_rx1_pattern[4], sizeof(T), 0xEE);

    set_buffer(&p_rx2_pattern[0], sizeof(T), 0x11);
    set_buffer(&p_rx2_pattern[1], sizeof(T), 0x22);
    set_buffer(&p_rx2_pattern[2], sizeof(T), 0x33);
    set_buffer(&p_rx2_pattern[3], sizeof(T), 0x44);
    set_buffer(&p_rx2_pattern[4], sizeof(T), 0x55);

    set_buffer(p_fill_symbol, sizeof(T), 0xFF);

    /* Exception: master TX > master RX . */
    if (config->master_tx_cnt > config->master_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);


    }
    /* Exception: master TX < master RX . */
    if (config->master_tx_cnt < config->master_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave TX > slave RX . */
    if (config->slave_tx_cnt > config->slave_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);
    }
    /* Exception: slave TX < slave RX . */
    if (config->slave_tx_cnt < config->slave_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: master TX buffer undefined . */
    if (!config->master_tx_defined) {
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

    /* Exception: master RX buffer undefined . */
    if (!config->master_rx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0x11);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0x22);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0x33);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x44);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x55);
    }

    /* Exception: slave TX buffer undefined . */
    if (!config->slave_tx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave RX buffer undefined . */
    if (!config->slave_rx_defined) {
        set_buffer(&p_rx1_pattern[0], sizeof(T), 0xAA);
        set_buffer(&p_rx1_pattern[1], sizeof(T), 0xBB);
        set_buffer(&p_rx1_pattern[2], sizeof(T), 0xCC);
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xDD);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xEE);
    }

    /* Handle symbol size. */
    T sym_mask = ((1 << config->symbol_size) - 1);

    for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
        p_tx_pattern[i] = (p_tx_pattern[i] & sym_mask);
        p_rx1_pattern[i] = (p_rx1_pattern[i] & sym_mask);
        p_rx2_pattern[i] = (p_rx2_pattern[i] & sym_mask);
    }

    memcpy(p_tx_buff, p_tx_pattern, sizeof(T) * TEST_SYM_CNT);
    set_buffer(p_rx_buff, sizeof(T) * TEST_SYM_CNT, 0x00);
}


template<typename T>
int slave_transfer(spi_t *obj, config_test_case_t *config)
{
    int status = CMDLINE_RETCODE_SUCCESS;
    bool test_passed;
    uint32_t count;

    uint32_t clocked_symbols_1 = TEST_SYM_CNT;
    uint32_t clocked_symbols_2 = TEST_SYM_CNT;

    if (config->duplex != FULL_DUPLEX) {
        clocked_symbols_1 = (config->slave_tx_cnt + config->slave_rx_cnt);
        clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);
    }

    T tx_pattern[TEST_SYM_CNT];
    T rx1_pattern[TEST_SYM_CNT];
    T rx2_pattern[TEST_SYM_CNT];
    T tx_buff[TEST_SYM_CNT];
    T rx_buff[TEST_SYM_CNT];
    T fill_symbol;

    void *p_tx_buff = tx_buff;
    void *p_rx_buff = rx_buff;

    if (!config->slave_tx_defined) {
        p_tx_buff = NULL;
    }

    if (!config->slave_rx_defined) {
        p_rx_buff = NULL;
    }

    init_transmission_buffers<T>(config, &tx_pattern[0], &rx1_pattern[0], &rx2_pattern[0], &tx_buff[0], &rx_buff[0], &fill_symbol);

    test_passed = true;
    wait_before_transmission();
    led = 1;

    count = sync_async_transfer(obj, p_tx_buff, config->slave_tx_cnt, p_rx_buff, config->slave_rx_cnt, &fill_symbol, config->sync);

    if (!check_buffers(rx1_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT)) {
        printf("ERROR (T1): Slave RX buffer invalid. \r\n ");
        test_passed = false;
    }

    if (!check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT)) {
        printf("ERROR (T1): Slave TX buffer invalid. \r\n ");
        test_passed = false;
    }

    if (clocked_symbols_1 != count) {
        printf("ERROR (T1): Slave Clocked symbol count invalid. \r\n ");
        test_passed = false;
    }

    if(!test_passed) {
        dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);
    }

    /* Send data received from master in the previous transmission if possible. */
    if (p_tx_buff && p_rx_buff) {
        memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
    }

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    test_passed = true;
    wait_before_transmission();

    count = sync_async_transfer(obj, p_tx_buff, TEST_SYM_CNT, p_rx_buff, TEST_SYM_CNT, &fill_symbol, config->sync);

    if (!check_buffers(rx1_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT)) {
        printf("ERROR (T1): Slave RX buffer invalid. \r\n ");
        test_passed = false;
    }

    if (!check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT)) {
        printf("ERROR (T1): Slave TX buffer invalid. \r\n ");
        test_passed = false;
    }

    if (clocked_symbols_2 != count) {
        printf("ERROR (T2): Slave Clocked symbol count invalid. \r\n ");
        test_passed = false;
    }

    if(!test_passed) {
        dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);
    }

    return status;
}

/* test_init command. */
int test_init_slave(spi_t * obj, config_test_case_t *config)
{
    spi_capabilities_t capabilities = { 0 };

    led = 0;

    spi_get_capabilities(spi_get_module(MBED_CONF_APP_SPI_SLAVE_MOSI,
                                        MBED_CONF_APP_SPI_SLAVE_MISO,
                                        MBED_CONF_APP_SPI_SLAVE_CLK),
                         MBED_CONF_APP_SPI_SLAVE_CS,
                         &capabilities);

    PinName miso = MBED_CONF_APP_SPI_SLAVE_MISO;
    PinName mosi = MBED_CONF_APP_SPI_SLAVE_MOSI;

    /* Adapt Full duplex/Half duplex settings. */
    switch (config->duplex)
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
    spi_init(obj, true, mosi, miso, MBED_CONF_APP_SPI_SLAVE_CLK, MBED_CONF_APP_SPI_SLAVE_CS);

    spi_format(obj, config->symbol_size, config->mode, config->bit_ordering);

    return CMDLINE_RETCODE_SUCCESS;
}

/* test_transfer command. */
int test_transfer_slave(spi_t * obj,config_test_case_t *config)
{
    int status = CMDLINE_RETCODE_SUCCESS;

    if (config->symbol_size <= 8) {
        status = slave_transfer<uint8_t>(obj, config);
    } else if (config->symbol_size <= 16) {
        status = slave_transfer<uint16_t>(obj, config);
    } else {
        status = slave_transfer<uint16_t>(obj, config);
    }

    return status;
}

/* test_finish command. */
int test_finish_slave(spi_t * obj, config_test_case_t *config)
{
    spi_free(obj);

    return CMDLINE_RETCODE_SUCCESS;
}

