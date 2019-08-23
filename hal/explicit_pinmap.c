
/** \addtogroup hal */
/** @{*/
/* mbed Microcontroller Library
 * Copyright (c) 2006-2019 ARM Limited
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

#include "mbed_error.h"
#include "spi_api.h"
#include "pwmout_api.h"
#include "analogin_api.h"
#include "analogout_api.h"
#include "i2c_api.h"
#include "serial_api.h"

#if DEVICE_SPI
MBED_WEAK void spi_init_direct(spi_t *obj, const spi_pinmap_t *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for SPI is unsupported on this device.");
}
#endif

#if DEVICE_PWMOUT
MBED_WEAK void pwmout_init_direct(pwmout_t* obj, const PinMap *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for PWM is unsupported on this device.");
}
#endif

#if DEVICE_ANALOGIN
MBED_WEAK void analogin_init_direct(analogin_t* obj, const PinMap *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for Analogin is unsupported on this device.");
}
#endif

#if DEVICE_ANALOGOUT
MBED_WEAK void analogout_init_direct(dac_t* obj, const PinMap *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for Analogin is unsupported on this device.");
}
#endif

#if DEVICE_I2C
MBED_WEAK void i2c_init_direct(i2c_t *obj, const i2c_pinmap_t *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for I2C is unsupported on this device.");
}
#endif

#if DEVICE_SERIAL
MBED_WEAK void serial_init_direct(serial_t *obj, const serial_pinmap_t *pinmap)
{
    /* Silent the compiler. */
    (void) obj;
    (void) pinmap;

    /* By default explicit pinmap is unsupported. */
    MBED_ERROR(MBED_ERROR_UNSUPPORTED, "Explicit pinmap for Serial is unsupported on this device.");
}
#endif