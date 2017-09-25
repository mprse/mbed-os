/** \addtogroup hal */
/** @{*/
/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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
#ifndef MBED_US_TICKER_API_H
#define MBED_US_TICKER_API_H

#include <stdint.h>
#include "hal/ticker_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hal_UsTicker Microseconds Ticker Functions
 * @{
 */

typedef void (*TICKER_IRQ_HANDLER_TYPE)(const ticker_data_t *const);

/** Set ticker IRQ handler
 *
 * @param ticker_irq_handler IRQ handler to be connected
 *
 */
void set_us_ticker_irq_handler(TICKER_IRQ_HANDLER_TYPE ticker_irq_handler);

/** Get ticker's data
 *
 * @return The microsecond ticker data
 */
const ticker_data_t* get_us_ticker_data(void);


/** The wrapper for ticker_irq_handler, to pass us ticker's data
 *
 */
void us_ticker_irq_handler(void);

/* HAL us ticker */

/** Initialize the ticker
 *
 * Initialize or re-initialize the ticker. This resets all the
 * clocking and prescaler registers, along with disabling
 * the compare interrupt.
 *
 * @note Initialization properties tested by ::ticker_init_test
 *
 * Pseudo Code:
 * @code
 * void us_ticker_init()
 * {
 *     // Enable clock gate so processor can read TIMER registers
 *     POWER_CTRL |= POWER_CTRL_TIMER_Msk;
 *
 *     // Disable the timer and ensure it is powered down
 *     TIMER_CTRL &= ~(TIMER_CTRL_ENABLE_Msk | TIMER_CTRL_COMPARE_ENABLE_Msk);
 *
 *     // Configure divisors
 *     uint32_t prescale = SystemCoreClock / 1000000;
 *     TIMER_PRESCALE = prescale - 1;
 *     TIMER_CTRL |= TIMER_CTRL_ENABLE_Msk;
 *
 *     // Install the interrupt handler
 *     NVIC_SetVector(TIMER_IRQn, (uint32_t)us_ticker_irq_handler);
 *     NVIC_EnableIRQ(TIMER_IRQn);
 * }
 * @endcode
 */
void us_ticker_init(void);

/** Read the current counter
 *
 * Read the current counter value without performing frequency conversions.
 *
 * @return The current timer's counter value in microseconds
 *
 * Pseudo Code:
 * @code
 * uint32_t us_ticker_read()
 * {
 *     return TIMER_COUNT;
 * }
 * @endcode
 */
uint32_t us_ticker_read(void);

/** Set interrupt for specified timestamp
 *
 * @param timestamp The time in ticks to be set
 *
 * @note no special handling needs to be done for times in the past
 * as the common timer code will detect this and call
 * ::us_ticker_fire_interrupt if this is the case
 *
 * Pseudo Code:
 * @code
 * void us_ticker_set_interrupt(timestamp_t timestamp)
 * {
 *     TIMER_COMPARE = timestamp;
 *     TIMER_CTRL |= TIMER_CTRL_COMPARE_ENABLE_Msk;
 * }
 * @endcode
 */
void us_ticker_set_interrupt(timestamp_t timestamp);

/** Disable us ticker interrupt
 *
 * Pseudo Code:
 * @code
 * void us_ticker_disable_interrupt(void)
 * {
 *     // Disable the compare interrupt
 *     TIMER_CTRL &= ~TIMER_CTRL_COMPARE_ENABLE_Msk;
 * }
 * @endcode
 */
void us_ticker_disable_interrupt(void);

/** Clear us ticker interrupt
 *
 * Pseudo Code:
 * @code
 * void us_ticker_clear_interrupt(void)
 * {
 *     // Write to the ICR (interrupt clear register) of the TIMER
 *     TIMER_ICR = TIMER_ICR_COMPARE_Msk;
 * }
 * @endcode
 */
void us_ticker_clear_interrupt(void);

/** Set pending interrupt that should be fired right away.
 * 
 * The ticker should be initialized prior calling this function.
 *
 * Pseudo Code:
 * @code
 * void us_ticker_fire_interrupt(void)
 * {
 *     NVIC_SetPendingIRQ(TIMER_IRQn);
 * }
 * @endcode
 */
void us_ticker_fire_interrupt(void);

/** Get frequency and counter bits of this ticker.
 *
 * Pseudo Code:
 * @code
 * const ticker_info_t* us_ticker_get_info()
 * {
 *     static const ticker_info_t info = {
 *         1000000,    // 1 MHz
 *         32          // 32 bit counter
 *     };
 *     return &info;
 * }
 * @endcode
 */
const ticker_info_t* us_ticker_get_info(void);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

/** @}*/
