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
#include "lp_ticker_api.h"
#include "common_rtc.h"

#if DEVICE_LOWPOWERTIMER

void lp_ticker_init(void)
{
    common_rtc_init();
}

uint32_t lp_ticker_read()
{
    return common_rtc_32bit_ticks_get();
}

void lp_ticker_set_interrupt(timestamp_t timestamp)
{
    common_rtc_set_interrupt(timestamp,
        LP_TICKER_CC_CHANNEL, LP_TICKER_INT_MASK);
}

void lp_ticker_fire_interrupt(void)
{
    m_common_rtc_interrupt_fire = true;

    NVIC_SetPendingIRQ(RTC1_IRQn);
}

void lp_ticker_disable_interrupt(void)
{
    nrf_rtc_int_disable(COMMON_RTC_INSTANCE, LP_TICKER_INT_MASK);
}

void lp_ticker_clear_interrupt(void)
{
    nrf_rtc_event_clear(COMMON_RTC_INSTANCE, LP_TICKER_EVENT);
}

#endif // DEVICE_LOWPOWERTIMER
