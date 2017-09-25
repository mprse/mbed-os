
/** \addtogroup hal */
/** @{*/
/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
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
#ifndef MBED_LPTICKER_API_H
#define MBED_LPTICKER_API_H

#include "device.h"

#if DEVICE_LOWPOWERTIMER

#include "hal/ticker_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hal_LpTicker Low Power Ticker Functions
 * @{
 */

/** Get low power ticker's data
 *
 * @return The low power ticker data
 */
const ticker_data_t* get_lp_ticker_data(void);

/** The wrapper for ticker_irq_handler, to pass lp ticker's data
 *
 */
void lp_ticker_irq_handler(void);

/* HAL lp ticker */

/** Initialize the low power ticker
 *
 * Pseudo Code:
 * @code
 * void lp_ticker_init()
 * {
 *     // Enable clock gate so processor can read LPTMR registers
 *     POWER_CTRL |= POWER_CTRL_LPTMR_Msk;
 *
 *     // Disable the timer and ensure it is powered down
 *     LPTMR_CTRL &= ~(LPTMR_CTRL_ENABLE_Msk | LPTMR_CTRL_COMPARE_ENABLE_Msk);
 *
 *     // Configure divisors - no division necessary
 *     LPTMR_PRESCALE = 0;
 *     LPTMR_CTRL |= LPTMR_CTRL_ENABLE_Msk;
 *
 *     // Install the interrupt handler
 *     NVIC_SetVector(LPTMR_IRQn, (uint32_t)lp_ticker_irq_handler);
 *     NVIC_EnableIRQ(LPTMR_IRQn);
 * }
 * @endcode
 */
void lp_ticker_init(void);

/** Read the current counter
 *
 * @return The current timer's counter value in ticks
 *
 * Pseudo Code:
 * @code
 * uint32_t lp_ticker_read()
 * {
 *     uint16_t count;
 *     uint16_t last_count;
 *
 *     // Loop until the same tick is read twice since this
 *     // is ripple counter on a different clock domain.
 *     count = LPTMR_COUNT;
 *     do {
 *         last_count = count;
 *         count = LPTMR_COUNT;
 *     } while (last_count != count);
 *
 *     return count;
 * }
 * @endcode
 */
uint32_t lp_ticker_read(void);

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
 * void lp_ticker_set_interrupt(timestamp_t timestamp)
 * {
 *     LPTMR_COMPARE = timestamp;
 *     LPTMR_CTRL |= LPTMR_CTRL_COMPARE_ENABLE_Msk;
 * }
 * @endcode
 */
void lp_ticker_set_interrupt(timestamp_t timestamp);

/** Disable low power ticker interrupt
 *
 * Pseudo Code:
 * @code
 * void lp_ticker_disable_interrupt(void)
 * {
 *     // Disable the compare interrupt
 *     LPTMR_CTRL &= ~LPTMR_CTRL_COMPARE_ENABLE_Msk;
 * }
 * @endcode
 */
void lp_ticker_disable_interrupt(void);

/** Clear the low power ticker interrupt
 *
 * Pseudo Code:
 * @code
 * void lp_ticker_clear_interrupt(void)
 * {
 *     // Write to the ICR (interrupt clear register) of the LPTMR
 *     LPTMR_ICR = LPTMR_ICR_COMPARE_Msk;
 * }
 * @endcode
 */
void lp_ticker_clear_interrupt(void);

/** Set pending interrupt that should be fired right away.
 * 
 * Pseudo Code:
 * @code
 * void lp_ticker_fire_interrupt(void)
 * {
 *     NVIC_SetPendingIRQ(LPTMR_IRQn);
 * }
 * @endcode
 */
void lp_ticker_fire_interrupt(void);

/** Get frequency and counter bits of this ticker.
 *
 * Pseudo Code:
 * @code
 * const ticker_info_t* lp_ticker_get_info()
 * {
 *     static const ticker_info_t info = {
 *         32768,      // 32KHz
 *         16          // 16 bit counter
 *     };
 *     return &info;
 * }
 * @endcode
 */
const ticker_info_t* lp_ticker_get_info(void);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

/** @}*/
