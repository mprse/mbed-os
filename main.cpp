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

spi_t spi;

int main()
{
    int pins_function[4] = {2, 2, 2, 2};
    PinName pins[4] = {PTD2, PTD3, PTD1, PTD0};
    explicit_pinmap_t explicit_spi_pinmap = {SPI_0, pins_function};

    //SPI spi(PTD2, PTD3, PTD1, PTD0);
    SPI spi(PTD2, PTD3, PTD1, PTD0, &explicit_spi_pinmap);
    spi.format(8, 0);
    spi.frequency(1000000);

    while(1);

    return 0;
}
