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

#ifndef MBED_MANAGER_TESTS_H
#define MBED_MANAGER_TESTS_H

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
 *     mbed test -t <toolchain> -m <target> -n tests-mbed_hal-sleepmanager*
 *
 * # Defined behavior
 * 
 * * Sleep manager functions are IRQ and thread safe - Verified by counter, multithread and irq
 *   tests - ::sleep_manager_deepsleep_counter_test, ::sleep_manager_multithread_test,
 *   ::sleep_manager_irq_test
 * @{
 */

/** Sleep deep counter increase/decrease test
 */
void sleep_manager_deepsleep_counter_test();

/** 2 threads that are locking/unlocking deep sleep
 */
void sleep_manager_multithread_test();

/** One loop with one IRQ that are locking/unlocking deep sleep
 */
void sleep_manager_irq_test();

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

