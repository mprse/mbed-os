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

#define BAUD_RATE_REQUIRED 9600

const char TEST_STRING[] = "Green Tea UART bear metal test.";
const int  TEST_STRING_LEN = 31;

RawSerial serial(USBTX, USBRX, BAUD_RATE_REQUIRED);

DigitalIn btn(BUTTON1);
DigitalOut led(LED1);

/* Send 100 chars in loop. */
void test_putc()
{
    for (int i=0; i < TEST_STRING_LEN; i++)
    {
        serial.putc(TEST_STRING[i]);
    }
}

int main() {
    volatile bool send = true;

    while(1)
    {
        if(btn == 0 && send)
        {
            led = 0;
            test_putc();
            send = false;
        }

        if(btn == 1)
        {
            led = 1;
            send = true;
        }
    }

}
