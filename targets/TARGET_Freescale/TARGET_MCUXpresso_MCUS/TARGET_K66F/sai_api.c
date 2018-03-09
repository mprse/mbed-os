/* mbed Microcontroller Library
 * Copyright (c) 2006-2018 ARM Limited
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
#include "sai_api.h"

#if DEVICE_SAI

// math.h required for floating point operations for baud rate calculation
#include <math.h>
#include "mbed_assert.h"

#include <string.h>

#include "cmsis.h"
#include "pinmap.h"
#include "fsl_sai.h"
#include "peripheral_clock_defines.h"
#include "PeripheralPins.h"
#include "fsl_clock_config.h"

static I2S_Type *const sai_addrs[] = I2S_BASE_PTRS;

const sai_format_t k66f_pcm_mode = {
    .sample_rate = 16000,
    .bclk_polarity = true,
    .wclk_polarity = true,
    .ws_delay = false,
    .ws_length = 32,
    .frame_length = 1,
    .word_mask = 0,
    .word_length = 32,
    .data_length = 16,
    .lsb_first = false,
    .aligned_left = true,
    .bit_shift = 0
};


/** Initialize `obj` based on `init` values.
 * This function may fail if the underlying peripheral does not support the requested features.
 * @return `false` on failure.
 */
bool sai_init(sai_t *obj, sai_init_t *init) {
    int32_t base = NC;
    int32_t tx = NC;
    int32_t rx = NC;

    MBED_ASSERT(init != NULL);
    if (!init->TX.enable && !init->RX.enable) {
        // we need at least 1 of them
        return false;
    }
    if (memcmp(&(init->TX.format), &k66f_pcm_mode, sizeof(sai_format_t)) != 0) {
        // we only support 1 format so far
        return false;
    }

    // validate pins & fetch base peripheral address
    int32_t mclk = pinmap_peripheral(init->mclk, PinMap_SAI_MCLK);
    if (init->TX.enable) {
        int32_t sd = pinmap_peripheral(init->TX.sd, PinMap_SAI_TXSD);
        int32_t bclk = pinmap_peripheral(init->TX.bclk, PinMap_SAI_TXBCLK);
        int32_t wclk = pinmap_peripheral(init->TX.wclk, PinMap_SAI_TXWCLK);
        tx = pinmap_merge(pinmap_merge(sd, bclk), wclk);
        MBED_ASSERT(tx != NC);
    }
    if (init->RX.enable) {
        int32_t sd = pinmap_peripheral(init->RX.sd, PinMap_SAI_RXSD);
        int32_t bclk = pinmap_peripheral(init->RX.bclk, PinMap_SAI_RXBCLK);
        int32_t wclk = pinmap_peripheral(init->RX.wclk, PinMap_SAI_RXWCLK);
        rx = pinmap_merge(pinmap_merge(sd, bclk), wclk);
        MBED_ASSERT((rx != NC) && (sd != NC) && (bclk != NC) && (wclk != NC));
    }
    base = pinmap_merge(pinmap_merge(tx, rx), mclk);
    MBED_ASSERT(base != NC);

    obj->base = sai_addrs[base];

    uint32_t mclk_freq = CLOCK_GetFreq(kCLOCK_CoreSysClk);
    // enable recv
    sai_config_t config;
    sai_transfer_format_t format;
    
    if (init->TX.enable) {
        if (init->TX.sd != NC) {
            pinmap_pinout(init->TX.sd, PinMap_SAI_TXSD);
        }
        if (init->TX.bclk != NC) {
            pinmap_pinout(init->TX.bclk, PinMap_SAI_TXBCLK);
        }
        if (init->TX.wclk != NC) {
            pinmap_pinout(init->TX.wclk, PinMap_SAI_TXWCLK);
        }
        obj->writable = true;

        SAI_TxGetDefaultConfig(&config);
        SAI_TxInit(obj->base, &config);

    }
    if (init->RX.enable) {
        if (init->RX.bclk != NC) {
            pinmap_pinout(init->RX.sd, PinMap_SAI_RXSD);
        }
        if (init->RX.bclk != NC) {
            pinmap_pinout(init->RX.bclk, PinMap_SAI_RXBCLK);
        }
        if (init->RX.bclk != NC) {
            pinmap_pinout(init->RX.wclk, PinMap_SAI_RXWCLK);
        }
        obj->readable = true;
        /* Initialize SAI Rx */
        SAI_RxGetDefaultConfig(&config);
        SAI_RxInit(obj->base, &config);
    }

    /* Configure the audio format */
    format.bitWidth = kSAI_WordWidth16bits;
    format.channel = 0U;
    format.sampleRate_Hz = 16000;
    format.masterClockHz = 384U * format.sampleRate_Hz;

    format.protocol = config.protocol;
    format.stereo = kSAI_Stereo;
    // FSL_FEATURE_SAI_FIFO_COUNT = 8 on MK66F18
    format.watermark = FSL_FEATURE_SAI_FIFO_COUNT / 2U;

    if (obj->writable) { SAI_TxSetFormat(obj->base, &format, mclk_freq, format.masterClockHz); }
    if (obj->readable) { SAI_RxSetFormat(obj->base, &format, mclk_freq, format.masterClockHz); }

    return true;
}

/** Transfer a sample and return the sample received meanwhile. */
uint32_t sai_xfer(sai_t *obj, uint32_t channel, uint32_t sample) {

    if (obj->writable) {
        SAI_TxEnable(obj->base, true);
        while (!(SAI_TxGetStatusFlag(obj->base) & kSAI_FIFOWarningFlag)) ;
        SAI_WriteData(obj->base, channel, sample);
    }
    
    if (!obj->readable) { return 0; }
    
    SAI_RxEnable(obj->base, true);    
    return SAI_ReadData(obj->base, channel);
}

/** Changes the format of both transmitter & receiver */
bool sai_configure_format(sai_t *obj, sai_format_t *fmt) {
    (void) obj;
    (void) fmt;
    return false;
}

/** Releases & de-initialize the referenced peripherals. */
void sai_free(sai_t *obj) {
    if (obj->readable) { SAI_RxEnable(obj->base, false); }
    if (obj->writable) { SAI_TxEnable(obj->base, false); }
}
#endif
