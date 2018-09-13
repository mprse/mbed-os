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

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "mbed.h"
#include "mbed_mktime.h"

#define LAST_VALID_YEAR 206

using namespace utest::v1;


int main()
{
    int wakeUpCount = 0;
    time_t waitStartTime = time(NULL);
    time_t now;
    int x;

    printf("Starting up.\r\n");
    //set_time(1);
    //waitStartTime = time(NULL);

    while (1) {
        now = time(NULL);
        x = now - waitStartTime;

        wakeUpCount++;
        printf("%d: started wait at %lu, time now %lu, difference %lu.\r\n",
               wakeUpCount, waitStartTime, now, x);
        waitStartTime = time(NULL);
        wait_ms(10000);
    }
}

void test_local_time_invalid_param()
{
}

Case cases[] = {
    Case("test is leap year - RTC leap years full support", test_local_time_invalid_param),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(20, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int mainx()
{
    return Harness::run(specification);
}
