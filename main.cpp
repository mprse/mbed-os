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

void test_spi()
{
    /* Regular use (master) */
    //SPI spi(D1, D2, D3, D4);

    /* Explicit pinmap */
    //const spi_pinmap_t explicit_spi_pinmap = {SPI_1, D1, 2, D2, 2, D3, 3, D4, 4};
    //SPI spi(explicit_spi_pinmap);

    //spi.format(8,0);

    //while(1);
}

void test_pwm()
{
    //PwmOut led(LED1);

    const PinMap explicit_pinmap = {D1, 0, 0};
    PwmOut led(explicit_pinmap);

    led.period(4.0f);      // 4 second period
    led.write(0.50f);      // 50% duty cycle, relative to period
    while(1);
}

void test_analogin()
{
    //AnalogIn ain(D1);

    const PinMap explicit_pinmap = {D1, 0, 0};
    AnalogIn ain(explicit_pinmap);

    if(ain > 0.3f) {
        printf("aa");
    }

    while(1);
}

void test_analogout()
{
    //AnalogOut  aout(D1);

    const PinMap explicit_pinmap = {D1, 0, 0};
    AnalogOut aout(explicit_pinmap);

    aout = 0.1;

    while(1);
}

void test_i2c()
{
    //I2C i2c(D1, D2);

    const i2c_pinmap_t explicit_pinmap = {0, D1, 0, D2, 0};
    I2C i2c(explicit_pinmap);

    i2c.frequency(1000000);

    while(1);
}

void test_serial()
{
    //Serial serial(D1, D2);

    const serial_pinmap_t explicit_pinmap = {0, D1, 0, D2, 0, 0};
    Serial serial(explicit_pinmap);

    if (serial.readable()) {
        printf("a");
    }

    while(1);
}

int main()
{
    //test_spi();

    //test_pwm();

    //test_analogin();

    //test_analogout();

    //test_i2c();

    test_serial();

    return 0;
}
