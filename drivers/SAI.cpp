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

    if (_is_input) {
        init.mclk = SAI_RX_MCLK;
        init.sd = SAI_RX_SD;
        init.bclk = SAI_RX_BCLK;
        init.wclk = SAI_RX_WCLK;
    } else {
        init.mclk = SAI_TX_MCLK;
        init.sd = SAI_TX_SD;
        init.bclk = SAI_TX_BCLK;
        init.wclk = SAI_TX_WCLK;
    }

    init.is_receiver = is_input;
    init.sample_rate = 8000;
    init.mclk_source = SAI_CLOCK_SOURCE_Internal;
    init.input_mclk_frequency = 0; // 0 means find it by yourself.
    init.output_mclk_frequency = 256 * init.sample_rate;

    init.format = *fmt;

    if (sai_init(&_sai, &init) != SAI_RESULT_OK) {
        printf("woops\r\n");
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
