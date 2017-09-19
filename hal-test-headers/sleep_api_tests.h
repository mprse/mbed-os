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

/** \addtogroup hal */
/** @{*/

#ifndef MBED_SLEEP_API_TESTS_H
#define MBED_SLEEP_API_TESTS_H

#include "device.h"

#if DEVICE_SLEEP

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hal_sleep_tests sleep hal requirements and tests
 * The sleep HAL tests ensure driver conformance to defined behavior.
 *
 * To run the sleep hal tests use the command:
 *
 *     mbed test -t <toolchain> -m <target> -n tests-mbed_hal-sleep*
 *
 * # Defined behavior
 * 
 * * Sleep mode 
 *   * wake-up time should be less than 10 us - Verified by ::sleep_usticker_test
 *   * all peripherals operate as in run mode 
 * * Deep sleep
 *   * High-speed clocks are turned off
 *   * lp ticker should wake up a target from this mode - Verified by ::deepsleep_lpticker_test
 *   * GPIO should wake up a target from this mode
 *   * The wake-up time should be less than 5 ms - Verified by ::deepsleep_lpticker_test
 *   * RTC keeps time
 *   
 * # Undefined behavior
 *
 * * peripherals aside from RTC, GPIO and lp ticker result in undefined behavior in deep sleep
 * @{
 */

/** Microsecond ticker interrupt to wake up from sleep (locked deepsleep)
 */
void sleep_usticker_test();


/** Low power ticker interrupt to wake up from deep sleep (unlocked deepsleep)
 */
void deepsleep_lpticker_test();

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

