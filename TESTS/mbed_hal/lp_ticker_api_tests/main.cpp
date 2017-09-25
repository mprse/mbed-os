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

void lp_ticker_info_test()
{
    const ticker_data_t* lp_ticker_data = get_lp_ticker_data();
    const ticker_info_t* p_lp_ticker_info = lp_ticker_get_info();

    printf("----> freq: %d \n", (int)p_lp_ticker_info->frequency);
    printf("----> bits: %d \n", (int)p_lp_ticker_info->bits);
}


utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(10, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Test calloc", lp_ticker_info_test),
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
