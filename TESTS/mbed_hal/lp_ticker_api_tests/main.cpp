/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
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
#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "rtos.h"

#if !DEVICE_LOWPOWERTIMER
#error [NOT_SUPPORTED] test not supported
#endif

using namespace utest::v1;

volatile int intFlag = 0;

#define TICKER_INT_VAL 5000
#define TICKER_DELTA 50

void ticker_event_handler_stub(const ticker_data_t *const ticker)
{
    /* Indicate that ISR has been executed in interrupt context. */
    if(IS_IRQ_MODE()) {
        intFlag++;
    }

    /* Clear and disable ticker interrupt. */
    lp_ticker_clear_interrupt();
    lp_ticker_disable_interrupt();
}

/* Test that the ticker has the right frequency and number of bits.
 *
 * Given ticker is available.
 * When ticker information data is obtained.
 * Then ticker information indicate that frequency frequency between 8KHz and 64KHz and the counter is at least 12 bits wide.
 */
void lp_ticker_info_test()
{
    const ticker_info_t* p_ticker_info = lp_ticker_get_info();

    /* Check conditions. */
    TEST_ASSERT(p_ticker_info->frequency >= 8000);
    TEST_ASSERT(p_ticker_info->frequency <= 64000);
    TEST_ASSERT(p_ticker_info->bits >= 12);
}

/* Test that the ticker continues operating in deep sleep mode.
 *
 * Given ticker is available.
 * When ticker has interrupt set and board enters deep-sleep mode.
 * Then ticker continues operating.
 */
void lp_ticker_sleep_test()
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Set IRQ handler for lp ticker. */
    set_lp_ticker_irq_handler(ticker_event_handler_stub);

    /* Init the ticker. */
    lp_ticker_init();

    /* Wait for green tea UART transmission before entering deep-sleep mode. */
    wait_ms(100);

    /* Get current tick count. */
    const uint32_t tick_count = lp_ticker_read();

    /* Set interrupt. Interrupt should be fired when tick count is equal to:
     * tick_count + TICKER_INT_VAL. */
    lp_ticker_set_interrupt(tick_count + TICKER_INT_VAL);

    /* Indicate that deep sleep mode is available. */
    TEST_ASSERT_TRUE(sleep_manager_can_deep_sleep());

    /* Enter deep-sleep mode. */
    while(!intFlag) {
        sleep();
    }

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(1, intFlag);
}

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(20, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("lp ticker info test", lp_ticker_info_test),
    Case("lp ticker sleep test", lp_ticker_sleep_test),
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
