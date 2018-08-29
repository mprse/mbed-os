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

#define TX_BUF_SIZE 10
#define RX_BUF_SIZE 10


/* mbed_retarget.h is included after errno.h so symbols are mapped to
 * consistent values for all toolchains */
#include "platform/mbed_retarget.h"

using namespace utest::v1;

void test_object()
{
    spi_t spi_slave;
    uint8_t tx_buff[TX_BUF_SIZE];
    uint8_t rx_buff[RX_BUF_SIZE];
    uint8_t fill_sym = 0x00;

    SPIName spi_name = spi_get_module(MBED_CONF_APP_SPI_MOSI, MBED_CONF_APP_SPI_MISO, MBED_CONF_APP_SPI_CLK);

    TEST_ASSERT(spi_name == SPI_0);

/* capabilities are unknown at the moment
    spi_capabilities_t caps;
    spi_get_capabilities(SPI_0, MBED_CONF_APP_SPI_CS, &caps);
    printf("Min freq: %u \n Max freq: %u \n word_length: 0x%8X \n slave: %c \n", caps.minimum_frequency, caps.maximum_frequency, caps.word_length, caps.support_slave_mode);
*/

    spi_init(&spi_slave, true, MBED_CONF_APP_SPI_MOSI, MBED_CONF_APP_SPI_MISO, MBED_CONF_APP_SPI_CLK, MBED_CONF_APP_SPI_CS);

    spi_format(&spi_slave, 8, SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE, SPI_BIT_ORDERING_MSB_FIRST);

    while(true) {
        spi_transfer(&spi_slave, tx_buff, TX_BUF_SIZE, rx_buff, RX_BUF_SIZE, fill_sym);

        printf("received: %s \n", tx_buff);
        printf("sending 'got it!' \n");

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
     Case("SPI - Object Definable", test_object,greentea_failure_handler),

 };

 Specification specification(test_setup, cases);

// // Entry point into the tests
int main() {
    return !Harness::run(specification);
}
