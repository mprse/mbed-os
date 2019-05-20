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
#include "hal/us_ticker_api.h"
#include "hal/lp_ticker_api.h"
#include "hal/mbed_lp_ticker_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "os_tick.h"
#ifdef __cplusplus
}
#endif // __cplusplus

using namespace utest::v1;

volatile int intFlag = 0;
const ticker_interface_t *intf;
ticker_irq_handler_type prev_irq_handler;

void overflow_protect()
{
    uint32_t time_window;
    time_window = 4000;

    const uint32_t ticks_now = intf->read();
    const ticker_info_t *p_ticker_info = intf->get_info();

    const uint32_t max_count = ((1 << p_ticker_info->bits) - 1);

    if ((max_count - ticks_now) > time_window) {
        return;
    }

    while (intf->read() > ticks_now);
}

void ticker_event_handler_stub(const ticker_data_t *const ticker)
{
    lp_ticker_clear_interrupt();
    intFlag++;
}


/* Test that ticker interrupt fires only when the ticker counter increments to the value set by ticker_set_interrupt. */
void ticker_interrupt_test(void)
{
    uint32_t ticker_timeout[] = { 5, 4, 3, 2, 1 };

    intf = get_lp_ticker_data()->interface;

    /* OS and common ticker may make use of lp ticker so suspend them for this test */
    osKernelSuspend();
    ticker_suspend(get_lp_ticker_data());

    intf->init();

    prev_irq_handler = set_lp_ticker_irq_handler(ticker_event_handler_stub);

    overflow_protect();

    for (uint32_t i = 0; i < (sizeof(ticker_timeout) / sizeof(uint32_t)); i++) {
        intFlag = 0;

        printf("Delay: %u ticks \r\n", ticker_timeout[i]);

        /* To be sure that we are not waiting for CMPOK */
        wait_ns(200000);

        const uint32_t tick_count = intf->read();

        intf->set_interrupt(tick_count + ticker_timeout[i]);

        while (intf->read() < (tick_count + 2*ticker_timeout[i])) {
            /* Just wait. */
        }

        TEST_ASSERT_EQUAL(1, intFlag);
    }

    set_lp_ticker_irq_handler(prev_irq_handler);

    ticker_resume(get_lp_ticker_data());
    osKernelResume(0);
}

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(80, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("lp ticker interrupt test", ticker_interrupt_test),

};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
