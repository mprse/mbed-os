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

SAI::SAI(const sai_format_t *fmt, bool is_input, uint32_t master_clock, bool internal_mclk) : _sai(), _mutex(), _is_input(is_input) {
    // No lock needed in the constructor
    sai_init_t init = {};
    init.is_receiver = is_input;
    init.is_slave = false;

    init.mclk = SAI_MCLK;
    if (_is_input) {
        init.sd = SAI_RX_SD;
        init.bclk = SAI_RX_BCLK;
        init.wclk = SAI_RX_WCLK;
    } else {
        init.sd = SAI_TX_SD;
        init.bclk = SAI_TX_BCLK;
        init.wclk = SAI_TX_WCLK;
    }

    init.mclk_internal_src = internal_mclk;
    init.mclk_freq = master_clock;  // 0 means guess it by yourself, else apply a clock div
    init.sample_rate = 16000;
    init.master_clock = 384 * init.sample_rate;

    init.format = *fmt;

    if (!sai_init(&_sai, &init)) {
        // log error
        // in true c++ we should throw an exception from here
        // but well... embedded c++
    }
}

bool SAI::xfer(uint32_t *value) {
    lock();
    bool ret = sai_xfer(&_sai, value);
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
