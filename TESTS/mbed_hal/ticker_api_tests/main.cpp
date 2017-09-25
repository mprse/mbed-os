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
#include "hal/us_ticker_api.h"
#include "core_cm.h"

#define FORCE_OVERFLOW_TEST (false)
#define TICKER_INT_VAL 5000
#define TICKER_DELTA 50

TICKER_OVERFLOW_DELTA 50
#if DEVICE_LOWPOWERTIMER
#define TICKER_OVERFLOW_DELTA 0 // this will allow to detect that ticker counter rollsovers to 0
#endif

#define MAX_FUNC_EXEC_TIME_US 20
#define DELTA_FUNC_EXEC_TIME_US 5
#define NUM_OF_CALLS 1000

#define NUM_OF_CYCLES 100000

using namespace utest::v1;

volatile int intFlag = 0;
const ticker_interface_t* intf;

/* Auxiliary function to count ticker ticks elapsed during execution of N cycles of empty while loop. */
uint32_t count_ticks(uint32_t cycles, uint32_t step)
{
    /* Init the ticker. */
    intf->init();

    core_util_critical_section_enter();

    const uint32_t start = intf->read();

    while(cycles -= step);

    const uint32_t stop = intf->read();

    core_util_critical_section_enter();

    return (stop - start);
}

void ticker_event_handler_stub(const ticker_data_t *const ticker)
{
    /* Indicate that ISR has been executed in interrupt context. */
    if(IS_IRQ_MODE()) {
        intFlag++;
    }

    /* Clear and disable ticker interrupt. */
    intf->clear_interrupt();
    intf->disable_interrupt();
}

/* Test that ticker_init can be called multiple times and
 * ticker_init resets the internal count and disables the ticker interrupt.
 *
 * Given ticker is initialised and interrupt is set.
 * When ticker is re-initialised.
 * Then ticker resets the internal count and disables the ticker interrupt.
 */
void ticker_init_test()
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Call init for the first time. */
    intf->init();

    /* Wait a while - let the ticker to count. */
    wait_ms(1);

    /* Set interrupt. */
    intf->set_interrupt(intf->read() + TICKER_INT_VAL);

    /* Get ticker count before re-initialisation. */
    const uint32_t ticks_before_reinit =  intf->read();

    /* Re-initialise the ticker. */
    intf->init();

    /* Get ticker count after re-initialisation. */
    const uint32_t ticks_after_reinit =  intf->read();

    /* Wait long enough to fire ticker interrupt (should not
     * be fired).
     */
    wait_ms(10);

    /* Check conditions. */
    TEST_ASSERT(ticks_before_reinit > ticks_after_reinit);
    TEST_ASSERT_EQUAL(0, intFlag);
}

/* Test that ticker frequency is non-zero and counter is at least 8 bits
 *
 * Given ticker is available.
 * When ticker information data is obtained.
 * Then ticker information indicate that frequency is non-zero and counter is at least 8 bits.
 */
void ticker_info_test(void)
{
    /* Get ticker info. */
    const ticker_info_t* p_ticker_info = intf->get_info();

    /* Check conditions. */
    TEST_ASSERT(p_ticker_info->frequency != 0);
    TEST_ASSERT(p_ticker_info->bits >= 8);
}

/* Test that ticker interrupt fires only when the ticker times increments to the value
 * set by ticker_set_interrupt.
 *
 * Given ticker is available, initialised.
 * When ticker interrupt is set.
 * Then ticker interrupt fires at the valid time.
 */
void ticker_interrupt_test(void)
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Init the ticker. */
    intf->init();

    /* Get current tick count. */
    const uint32_t tick_count = intf->read();

    /* Set interrupt. Interrupt should be fired when tick count is equal to:
     * tick_count + TICKER_INT_VAL. */
    intf->set_interrupt(tick_count + TICKER_INT_VAL);

    /* Wait until ticker count reach value: tick_count + TICKER_INT_VAL - TICKER_DELTA.
     * Interrupt should not be fired. */
    while(intf->read() < (tick_count + TICKER_INT_VAL - TICKER_DELTA)) {
        /* Indicate failure if interrupt has fired earlier. */
        if(intFlag != 0) {
            TEST_ASSERT(0);
        }
    }

    /* Wait until ticker count reach value: tick_count + TICKER_INT_VAL + TICKER_DELTA.
     * Interrupt should be fired after this time. */
    while(intf->read() < (tick_count + TICKER_INT_VAL + TICKER_DELTA)) {
        /* Just wait. */
    }

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(1, intFlag);
}

/* Test that ticker interrupt is not triggered when ticker_set_interrupt
 * is called with a time from the past
 *
 * Given ticker is available, initialised.
 * When ticker interrupt is set to the time in the past.
 * Then ticker interrupt is not triggered.
 */
void ticker_past_test(void)
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Init the ticker. */
    intf->init();

    /* Get current tick count. */
    const uint32_t tick_count = intf->read();

    /* Set interrupt tick count to value in the past.
     * Interrupt should be fired. */
    intf->set_interrupt(tick_count - TICKER_DELTA);

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(0, intFlag);
}

/* Test that ticker can be rescheduled repeatedly before the handler has been called.
 *
 * Given ticker is available, initialised.
 * When ticker interrupt is set and then rescheduled (interrupt time is modified).
 * Then ticker interrupt triggered according the rescheduled time.
 */
void ticker_repeat_reschedule_test(void)
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Init the ticker. */
    intf->init();

    /* Get current tick count. */
    const uint32_t tick_count = intf->read();

    /* Set interrupt. Interrupt should be fired when tick count is equal to:
     * tick_count + TICKER_INT_VAL. */
    intf->set_interrupt(tick_count + TICKER_INT_VAL);

    /* Reschedule interrupt - it should not be fired yet.
     * Call intf->set_interrupt again. */
    intf->set_interrupt(tick_count + 2*TICKER_INT_VAL);

    /* Wait until ticker count reach value: tick_count + 2*TICKER_INT_VAL - TICKER_DELTA.
     * Interrupt should not be fired. */
    while(intf->read() < (tick_count + 2 * TICKER_INT_VAL - TICKER_DELTA)) {
        /* Indicate failure if interrupt has fired earlier. */
        if(intFlag != 0) {
            TEST_ASSERT(0);
        }
    }

    /* Wait until ticker count reach value: tick_count + 2*TICKER_INT_VAL + TICKER_DELTA.
     * Interrupt should be fired after this time. */
    while(intf->read() < (tick_count + 2 * TICKER_INT_VAL + TICKER_DELTA)) {
        /* Just wait. */
    }

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(1, intFlag);
}

/* Test that ticker_fire_interrupt causes and interrupt to get fired immediately.
 *
 * Given ticker is available.
 * When ticker_fire_interrupt is called.
 * Then ticker interrupt is triggered.
 */
void ticker_fire_now_test(void)
{
    /* Clear ISR call counter. */
    intFlag = 0;

    /* Init the ticker. */
    intf->init();

    /* Fire the interrupt. */
    intf->fire_interrupt();

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(1, intFlag);
}

/* Test that the ticker correctly handles overflow.
 *
 * Note that for high frequency timers we will only prove that ticker counter rollovers and
 * continue counting (it is not possible to prove in deterministic way that after rollover
 * next value is 0).
 *
 * Given ticker is available.
 * When ticker has overflows.
 * Then ticker continues counting from the beginning and interrupt scheduling works.
 */
void ticker_overflow_test(void)
{
    /* Get ticker info. */
    const ticker_info_t* p_ticker_info = intf->get_info();

    /* We need to check how long it will take to overflow.
     * We will perform this test only if this time is no longer than 30 sec.
     */
    const uint32_t max_count = (1 << p_ticker_info->bits) - 1;
    const uint32_t required_time_sec = (max_count / p_ticker_info->frequency);

    if(required_time_sec > 30 && !FORCE_OVERFLOW_TEST) {
        TEST_ASSERT(true);
        return;
    }

    /* Clear ISR call counter. */
    intFlag = 0;

    /* Init the ticker. */
    intf->init();

    /* Wait for max count. */
    while(intf->read() != (max_count - TICKER_OVERFLOW_DELTA)) {
        /* Just wait. */
    }

    /* Now we are near the overflow point. Detect rollover. */
    while(intf->read() > TICKER_OVERFLOW_DELTA) {
        /* Just wait. */
    }

    /* Get ticker count after overflow. */
    const uint32_t after_overflow = intf->read();

    /* Now we are just after overflow. Wait a while assuming that ticker still counts. */
    while(intf->read() < TICKER_OVERFLOW_DELTA) {
        /* Just wait. */
    }

    /* Get ticker count. */
    const uint32_t next_after_overflow = intf->read();

    /* Check that after the overflow ticker counts again
     * from 0. */
    TEST_ASSERT(after_overflow <= TICKER_OVERFLOW_DELTA);
    TEST_ASSERT(next_after_overflow > TICKER_OVERFLOW_DELTA);
    TEST_ASSERT_EQUAL(0, intFlag);

    /* Get current tick count. */
    const uint32_t tick_count = intf->read();

    /* Check if interrupt scheduling still works. */
    intf->set_interrupt(tick_count + TICKER_INT_VAL);

    /* Wait for the interrupt. */
    while(intf->read() < (tick_count + TICKER_INT_VAL + TICKER_OVERFLOW_DELTA)) {
        /* Just wait. */
    }

    /* Check if interrupt has been fired. */
    TEST_ASSERT_EQUAL(1, intFlag);
}

/* Test that the ticker increments by one on each tick.
 *
 *
 * We have the following assumption for the timer clock frequency:
 *
 * NOTE:
 * high freq ticker: 250 KHz (1 tick per 4 us) - 8 Mhz (1 tick per 1/8 us)
 * low power ticker: 8 KHz (1 tikck per 125 us) - 64 KHz (1 tick per ~15.6 us)
 *
 * Lowest CPU speed is 16 MHz (1 tick per 1/16 us).
 *
 * For the boards with ticker clock freq less than or equal to 250 KHz we will try to prove that ticker is incremented by one
 * straightforward by reading ticker count value in the loop in order to detect a single ticker value update (hopefully by one).
 * For faster tickers we need to prove this indirectly using additional count_ticks() function which returns number of
 * ticks needed to perform N cycles of the empty while loop. For the same number of cycles function result should be the same with
 * accuracy +/- 1 tick. After the first test we will call count_ticks again with number of cycles equal N+1, N+2, ...
 * until we get other value of number of ticks.
 *
 * Given ticker is available.
 * When ticker is initialized.
 * Then ticker counter is incremented by one.
 */
void ticker_increment_test(void)
{
    /* Init the ticker. */
    intf->init();

    /* Get ticker info. */
    const ticker_info_t* p_ticker_info = intf->get_info();

    /* Perform test based on ticker speed. */
    if(p_ticker_info->frequency <= 250000) {    // low frequency tickers
        const uint32_t base_tick_count = intf->read();
        uint32_t next_tick_count = base_tick_count

        while((next_tick_count = intf->read()) == base_tick_count);

        TEST_ASSERT_EQUAL(base_tick_count + 1, next_tick_count);
    } else {                                    // high frequency tickers
    	const uint32_t base_tick_count count_ticks(NUM_OF_CYCLES, 1);
    	uint32_t next_tick_count = base_tick_count;
    	uint32_t inc_val = 0;

    	while(next_tick_count == base_tick_count) {
    		next_tick_count = count_ticks(NUM_OF_CYCLES + inc_val, 1);
    		inc_val++;
    	}

    	/* Since we are here we know that next_tick_count != base_tick_count.
    	 * The accuracy of our measurement method is +/- 1 tick, so it is possible that
    	 * next_tick_count == base_tick_count - 1. This is also valid result.
    	 */
    	TEST_ASSERT_UINT32_WITHIN(1, next_tick_count, base_tick_count);

    }
}

/* Test that common ticker functions complete with the required amount of time.
 *
 * Given ticker is available.
 * When ticker_read, ticker_clear_interrupt, ticker_set_interrupt, ticker_fire_interrupt or ticker_disable_interrupt function is called.
 * Then its execution is not longer than 20 us.
 */
void ticker_speed_test(void)
{
    Timer timer;
    int counter = NUM_OF_CALLS;

    /* Init the ticker. */
    intf->init();

    /* ---- Test ticker_read function. ---- */
    timer.reset();
    timer.start();
    while(counter--) {
        intf->read();
        }
    timer.stop();

    /* Check result. */
    TEST_ASSERT(timer.read_us() < (NUM_OF_CALLS * (MAX_FUNC_EXEC_TIME_US + DELTA_FUNC_EXEC_TIME_US)));

    /* ---- Test ticker_clear_interrupt function. ---- */
    timer.reset();
    timer.start();
    while(counter--) {
        intf->clear_interrupt();
        }
    timer.stop();

    /* Check result. */
    TEST_ASSERT(timer.read_us() < (NUM_OF_CALLS * (MAX_FUNC_EXEC_TIME_US + DELTA_FUNC_EXEC_TIME_US)));

    /* ---- Test ticker_set_interrupt function. ---- */
    timer.reset();
    timer.start();
    while(counter--) {
        intf->set_interrupt(0);
        }
    timer.stop();

    /* Check result. */
    TEST_ASSERT(timer.read_us() < (NUM_OF_CALLS * (MAX_FUNC_EXEC_TIME_US + DELTA_FUNC_EXEC_TIME_US)));

    /* ---- Test fire_interrupt function. ---- */
    timer.reset();
    timer.start();
    while(counter--) {
        intf->fire_interrupt();
        }
    timer.stop();

    /* Check result. */
    TEST_ASSERT(timer.read_us() < (NUM_OF_CALLS * (MAX_FUNC_EXEC_TIME_US + DELTA_FUNC_EXEC_TIME_US)));

    /* ---- Test disable_interrupt function. ---- */
    timer.reset();
    timer.start();
    while(counter--) {
        intf->disable_interrupt();
        }
    timer.stop();

    /* Check result. */
    TEST_ASSERT(timer.read_us() < (NUM_OF_CALLS * (MAX_FUNC_EXEC_TIME_US + DELTA_FUNC_EXEC_TIME_US)));
}


utest::v1::status_t hf_ticker_setup(const Case *const source, const size_t index_of_case)
{
    intf = get_us_ticker_data()->interface;

    set_us_ticker_irq_handler(ticker_event_handler_stub);

    return greentea_case_setup_handler(source, index_of_case);
}

#if DEVICE_LOWPOWERTIMER
utest::v1::status_t lp_ticker_setup(const Case *const source, const size_t index_of_case)
{
    intf = get_lp_ticker_data()->interface;

    set_lp_ticker_irq_handler(ticker_event_handler_stub);

    return greentea_case_setup_handler(source, index_of_case);
}
#endif

utest::v1::status_t test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(30, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
      //Case("hf ticker init is safe to call repeatedly", hf_ticker_setup, ticker_init_test),
      //Case("hf ticker frequency is non-zero and counter is at least 8 bits", hf_ticker_setup, ticker_info_test),
      //Case("hf ticker interrupt test", hf_ticker_setup, ticker_interrupt_test),
      //Case("hf ticker past interrupt test", hf_ticker_setup, ticker_past_test),
      //Case("hf ticker reschedule test", hf_ticker_setup, ticker_repeat_reschedule_test),
      //Case("hf ticker fire interrupt", hf_ticker_setup, ticker_fire_now_test),
      //Case("hf ticker overflow test", hf_ticker_setup, ticker_overflow_test),
      Case("hf ticker increment test", hf_ticker_setup, ticker_increment_test),
      //Case("hf ticker speed test", hf_ticker_setup, ticker_speed_test),
#if DEVICE_LOWPOWERTIMER
      //Case("lp ticker init is safe to call repeatedly", lp_ticker_setup, ticker_init_test),
      //Case("lp ticker frequency is non-zero and counter is at least 8 bits", lp_ticker_setup, ticker_info_test),
      //Case("lp ticker interrupt test", lp_ticker_setup, ticker_interrupt_test),
      //Case("lp ticker past interrupt test", lp_ticker_setup, ticker_past_test),
      //Case("lp ticker reschedule test", lp_ticker_setup, ticker_repeat_reschedule_test),
      //Case("lp ticker fire interrupt", lp_ticker_setup, ticker_fire_now_test),
      //Case("lp ticker overflow test", lp_ticker_setup, ticker_overflow_test),
      //Case("lp ticker increment test", lp_ticker_setup, ticker_increment_test),
      //Case("lp ticker speed test", lp_ticker_setup, ticker_speed_test),
#endif
};

Specification specification(test_setup, cases);

int main()
{
    return !Harness::run(specification);
}
