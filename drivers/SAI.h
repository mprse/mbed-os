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
#ifndef MBED_SAI_H
#define MBED_SAI_H

#include "platform/platform.h"

#if defined (DEVICE_SAI) || defined(DOXYGEN_ONLY)

#include "hal/sai_api.h"
#include "platform/NonCopyable.h"
#include "platform/PlatformMutex.h"

namespace mbed {
/** \addtogroup drivers */

/** A SAI Master, used for communicating with SAI slave devices
 *
 * @ingroup drivers
 */
class SAI : private NonCopyable<SAI> {

public:

    /** Create a SAI
     */
    SAI(const sai_format_t *fmt);

    /** Push a sample to the Fifo & try to read a new sample.
     * it may return 0 if no sample was available.
     */
    virtual uint32_t xfer(uint32_t channel, uint32_t value);

    /** Acquire exclusive access to this SAI bus
     */
    virtual void lock(void);

    /** Release exclusive access to this SAI bus
     */
    virtual void unlock(void);


public:
    virtual ~SAI() {
    }

protected:
    sai_t _sai;

    PlatformMutex _mutex;
};

} // namespace mbed

#endif

#endif
