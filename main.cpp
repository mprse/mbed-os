/* Copyright (c) 2018 Arm Limited
*
* SPDX-License-Identifier: Apache-2.0
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
#include "explicit_pinmap.h"

#ifndef RUN_EXP
#define RUN_EXP 1
#endif

#if DEVICE_SPI
static void test_spi()
{
#if !RUN_EXP
    /* Regular use (master) */
    SPI spi(D1, D2, D3, D4);
#else
    /* Explicit pinmap */
    const spi_pinmap_t explicit_spi_pinmap = {SPI_1, PTD2, 2, PTD3, 2, PTD1, 2, PTD0, 2}; // K64F
    //const spi_pinmap_t explicit_spi_pinmap = {P0_3, PTD2, 2, PTD3, 2, PTD1, 2, PTD0, 2}; // K64F

    //constexpr spi_pinmap_t explicit_spi_pinmap = get_spi_pinmap(PTD2, PTD3, PTD1, PTD0);  // NUCLEO_F429ZI
    //constexpr spi_pinmap_t explicit_spi_pinmap = get_spi_pinmap(PA_7, PA_6, PA_5, PA_4);    // NUCLEO_F411RE
    //constexpr spi_pinmap_t explicit_spi_pinmap = get_spi_pinmap(P0_3, P0_2, P0_6, P0_4);    // LPC

    SPI spi(explicit_spi_pinmap);
#endif
    spi.format(8,0);

    //while(1);
}
#endif

#if DEVICE_PWMOUT
static void test_pwm()
{
#if !RUN_EXP
    PwmOut led(LED1);
#else
    constexpr PinMap explicit_pinmap = get_pwm_pinmap(PTA1); // K64F
    //constexpr PinMap explicit_pinmap = get_pwm_pinmap(PA_0); // NUCLEO_F429ZI

    PwmOut led(explicit_pinmap);
#endif
    led.period(4.0f);      // 4 second period
    led.write(0.50f);      // 50% duty cycle, relative to period

    //while(1);
}
#endif

#if DEVICE_ANALOGIN
static void test_analogin()
{
#if !RUN_EXP
    AnalogIn ain(ADC_VREF);
#else
    constexpr PinMap explicit_pinmap = get_analogin_pinmap(PTB2);
    //constexpr PinMap explicit_pinmap = get_analogin_pinmap(PA_0);
    //constexpr PinMap explicit_pinmap = get_analogin_pinmap(P0_23);
    //constexpr PinMap explicit_pinmap = get_analogin_pinmap(p2);
    //constexpr PinMap explicit_pinmap = get_analogin_pinmap(ADC_VREF);

    AnalogIn ain(explicit_pinmap);
#endif
    if(ain > 0.3f) {
        printf("aa");
    }

    while(1);
}
#endif

#if DEVICE_ANALOGOUT
static void test_analogout()
{
#if !RUN_EXP
    AnalogOut  aout(D1);
#else
    constexpr PinMap explicit_pinmap = get_analogout_pinmap(DAC0_OUT);
    //constexpr PinMap explicit_pinmap = get_analogout_pinmap(PA_4);

    AnalogOut aout(explicit_pinmap);
#endif
    aout = 0.1;

    //while(1);
}
#endif

#if DEVICE_I2C
static void test_i2c()
{
#if !RUN_EXP
    I2C i2c(D1, D2);
#else
    constexpr i2c_pinmap_t explicit_pinmap = get_i2c_pinmap(PTB1, PTB0);
    //constexpr i2c_pinmap_t explicit_pinmap = get_i2c_pinmap(PB_9, PB_6);
    constexpr i2c_pinmap_t explicit_pinmap = get_i2c_pinmap(P0_13, P0_14);

    I2C i2c(explicit_pinmap);
#endif
    i2c.frequency(1000000);

    //while(1);
}
#endif

#if DEVICE_SERIAL
static void test_serial()
{
#if !RUN_EXP
    Serial serial(D1, D2);
    serial.set_flow_control(Serial::RTSCTS, D1, D2);
#else
    constexpr serial_pinmap_t explicit_pinmap = get_uart_pinmap(PA_0, PA_1);
    //constexpr serial_pinmap_t explicit_pinmap = get_uart_pinmap(P0_30, P0_29);
    constexpr serial_fc_pinmap_t explicit_pinmap_fc = get_uart_fc_pinmap(PA_0, PA_1);
    //constexpr serial_fc_pinmap_t explicit_pinmap_fc = get_uart_fc_pinmap(P1_8, P1_7);

    Serial serial(explicit_pinmap);

    serial.set_flow_control(Serial::RTSCTS, explicit_pinmap_fc);
#endif
    if (serial.readable()) {
        printf("a");
    }

    //while(1);
}
#endif

#if DEVICE_QSPI
static void test_qspi()
{
#if !RUN_EXP
    QSPI qspi_device(PB_1, PB_0, PA_7, PA_6, PB_10, PB_11);
#else
    constexpr qspi_pinmap_t explicit_pinmap = get_qspi_pinmap(PB_1, PB_0, PA_7, PA_6, PB_10, PB_11);
    QSPI qspi_device(explicit_pinmap);
    //qspi_device.configure_format(QSPI_CFG_BUS_SINGLE, QSPI_CFG_BUS_SINGLE,
    //                            QSPI_CFG_ADDR_SIZE_24, QSPI_CFG_BUS_SINGLE,
    //                            QSPI_CFG_ALT_SIZE_8, QSPI_CFG_BUS_SINGLE, 0);
#endif
    //while(1);
}
#endif

#if DEVICE_CAN
static void test_can()
{
    char counter;

#if !RUN_EXP
    CAN can(PA_11, PA_12);
#else
    //const serial_pinmap_t explicit_pinmap = {UART_0, PTB17, 3, PTB16, 3, 0};
    constexpr can_pinmap_t explicit_pinmap = get_can_pinmap(PA_11, PA_12);
    CAN can(explicit_pinmap, 10000);
#endif
    can.write(CANMessage(1337, &counter, 1));

    //while(1);
}
#endif

int main()
{
    //test_pwm();

    test_analogin();

    //test_analogout();

    //test_spi();

    //test_i2c();

    //test_serial();

    //test_qspi();

    //test_can();

    while(1);

    return 0;

}
