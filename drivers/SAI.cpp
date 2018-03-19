/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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
#include "drivers/SAI.h"
#include "platform/mbed_critical.h"

#if DEVICE_SAI

namespace mbed {

SAI::SAI(sai_init_t *sai_init_data) : _sai(), _mutex() {
    // No lock needed in the constructor
    if (!sai_init(&_sai, sai_init_data)) {
        printf("SAI INIT ERROR !!!! \n\n\n");
        // log error
        // in true c++ we should throw an exception from here
        // but well... embedded c++
    }
}

uint32_t SAI::xfer(uint32_t channel, uint32_t value) {
    lock();
    uint32_t ret = sai_xfer(&_sai, channel, value);
    unlock();
    return ret;
}

void SAI::lock() {
    _mutex.lock();
}

void SAI::unlock() {
    _mutex.unlock();
}

} // namespace mbed

#endif
