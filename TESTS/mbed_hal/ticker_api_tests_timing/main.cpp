/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/*
 * This is the mbed device part of the test to verify if mbed board ticker
 * freqency is valid.
 */

#include "mbed.h"
#include "greentea-client/test_env.h"
#include "utest/utest.h"
#include "unity/unity.h"

using namespace utest::v1;

const ticker_interface_t* intf;

void test_case_ticker_freq() {

    char _key[11] = { };
    char _value[128] = { };

    /* Get ticker info. */
    const ticker_info_t* p_ticker_info = us_ticker_get_info();

    /* Send defined ticker frequency to the host. */
    greentea_send_kv("ticker_clk_freq", p_ticker_info->frequency);

    /* Wait for start signal from the host. */
    greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

    /* Get current ticker tick count. */
    const uint32_t tick_count_start = intf->read();

    /* Wait for stop signal from the host. */
    greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

    /* Get current ticker tick count. */
    const uint32_t tick_count_stop = intf->read();

    /* Send number of counted ticks between start and stop to the host. */
    greentea_send_kv("total_tick_count", tick_count_stop - tick_count_start);

    /* Wait for the test result from the host. */
    greentea_parse_kv(_key, _value, sizeof(_key), sizeof(_value));

    /* Indicate test status. */
    TEST_ASSERT_MESSAGE("passed", _key);
}


utest::v1::status_t hf_ticker_case_setup_handler_t(const Case *const source, const size_t index_of_case) {
    intf = get_us_ticker_data()->interface;
    return greentea_case_setup_handler(source, index_of_case);
}

#if DEVICE_LOWPOWERTIMER
utest::v1::status_t lp_ticker_case_setup_handler_t(const Case *const source, const size_t index_of_case) {
    intf = get_lp_ticker_data()->interface;
    return greentea_case_setup_handler(source, index_of_case);
}
#endif

utest::v1::status_t ticker_case_teardown_handler_t(const Case *const source, const size_t passed, const size_t failed, const failure_t reason) {
    return greentea_case_teardown_handler(source, passed, failed, reason);
}


// Test cases
Case cases[] = {
    Case("hf ticker frequency test", hf_ticker_case_setup_handler_t, test_case_ticker_freq, ticker_case_teardown_handler_t),
#if DEVICE_LOWPOWERTIMER
    Case("lp ticker frequency test", lp_ticker_case_setup_handler_t, test_case_ticker_freq, ticker_case_teardown_handler_t),
#endif
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(120, "ticker_timing");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main() {
    Harness::run(specification);
}
