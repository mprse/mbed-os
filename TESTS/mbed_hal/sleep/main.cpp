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

#if !DEVICE_SLEEP
#error [NOT SUPPORTED] sleep not supported for this target
#endif

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "sleep_api_tests.h"

using namespace utest::v1;

// used for regular sleep modes, a target should be awake within 12 us (default 10 us + delta)
static const uint32_t sleep_mode_delta = 12;
// used for deepsleep, a target should be awake within 500 us (default 5ms + delta)
static const uint32_t deepsleep_mode_delta = 500;

void sleep_usticker_test()
{
    // test only sleep functionality
    sleep_manager_lock_deep_sleep();
    TEST_ASSERT_FALSE(sleep_manager_can_deep_sleep());

    // Testing period 10 us
    for (timestamp_t i = 10; i < 1000; i += 10) {
        timestamp_t next_match_timestamp = us_ticker_read() + i;
        us_ticker_set_interrupt(next_match_timestamp);
        sleep();
        TEST_ASSERT_UINT32_WITHIN(sleep_mode_delta, next_match_timestamp, us_ticker_read());
    }

    sleep_manager_unlock_deep_sleep();
    TEST_ASSERT_TRUE(sleep_manager_can_deep_sleep());
}

#ifdef DEVICE_LOWPOWERTIMER

void deepsleep_lpticker_test()
{
    TEST_ASSERT_TRUE_MESSAGE(sleep_manager_can_deep_sleep(), "deep sleep should not be locked");

    // Testing period 5 ms
    for (timestamp_t i = 5000; i < 100000; i += 5000) {
        timestamp_t next_match_timestamp = lp_ticker_read() + i;
        lp_ticker_set_interrupt(next_match_timestamp);
        sleep();
        TEST_ASSERT_UINT32_WITHIN(deepsleep_mode_delta, next_match_timestamp, lp_ticker_read());
    }
}

#endif

utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) 
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) 
{
    GREENTEA_SETUP(20, "default_auto");
    us_ticker_init();
#if DEVICE_LOWPOWERTIMER
    lp_ticker_init();
#endif
    // Suspend RTOS Kernel to enable sleep modes
    osKernelSuspend();
    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("sleep -  auto mode deepsleep locked, source of wake-up - us ticker", sleep_usticker_test, greentea_failure_handler),
#if DEVICE_LOWPOWERTIMER
    Case("sleep -  auto mode, source of wake-up - lp ticker",deepsleep_lpticker_test, greentea_failure_handler),
#endif
};

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main() {
    Harness::run(specification);
}
