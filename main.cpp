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
#if !defined(EXPLICIT_PINMAP)
    SPI spi(D0, D1, D2, D3);
    spi.format(8, 0);
    spi.frequency(1000000);
#else
    int pins_function[4] = {1, 2, 3, 4};
    explicit_pinmap_t explicit_spi_pinmap = {SPI_1, pins_function};

    SPI spi(D0, D1, D2, D3, &explicit_spi_pinmap);
    spi.format(8, 0);
    spi.frequency(1000000);
#endif
    while(1);

    return 0;
}
