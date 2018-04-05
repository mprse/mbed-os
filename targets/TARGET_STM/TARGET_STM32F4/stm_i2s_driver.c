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
 
#if DEVICE_SAI

#include <string.h>
#include "stm_i2s_driver.h"
#include "stm32f4xx_ll_spi.h"
#include "PeripheralPins.h"

static sai_result_t stm_i2s_init(sai_t *obj, sai_init_t *init);
static bool stm_i2s_xfer(sai_t *obj, uint32_t *sample);
static void stm_i2s_free(sai_t *obj);

const stm_sai_api_t stm_i2s_vtable = {
    .init = stm_i2s_init,
    .xfer = stm_i2s_xfer,
    .free = stm_i2s_free
};

static sai_result_t stm_i2s_init(sai_t *obj, sai_init_t *init) {
    if (memcmp(&(init->format), &sai_mode_i2s16w32, sizeof(sai_format_t)) != 0) {
        // we only support 1 format so far
        return SAI_RESULT_CONFIG_UNSUPPORTED;
    }
    if (init->sample_rate != 16000) {
        return SAI_RESULT_CONFIG_UNSUPPORTED;
    }
   
    // validate pins & fetch base peripheral address
    int32_t mclk = pinmap_peripheral(init->mclk, PinMap_SAI_MCLK);
    int32_t sd = pinmap_peripheral(init->sd, PinMap_SAI_SD);
    int32_t bclk = pinmap_peripheral(init->bclk, PinMap_SAI_BCLK);
    int32_t wclk = pinmap_peripheral(init->wclk, PinMap_SAI_WCLK);
    MBED_ASSERT((sd != NC) && (bclk != NC) && (wclk != NC));
    
    // should probably handle here I2Sxext 
    
    int32_t base = pinmap_merge(pinmap_merge(pinmap_merge(sd, bclk), wclk), mclk);
    MBED_ASSERT(base != NC);

    MBED_ASSERT((obj != NULL) && (obj->base != NULL) && (obj->base->type == SAI_PERIPH_TYPE_I2S));
    
    switch ((uint32_t)obj->base->u.spi) {
        case (uint32_t)SPI_2:
        case (uint32_t)I2S2ext:
            __HAL_RCC_SPI2_CLK_ENABLE();
            break;
        case (uint32_t)SPI_3:
        case (uint32_t)I2S3ext:
            __HAL_RCC_SPI3_CLK_ENABLE();
            break;
        default:
        return SAI_RESULT_CONFIG_UNSUPPORTED;
    }
    
    /* I2S GPIO Configuration --------------------------------------------------*/
    if (init->mclk != (PinName)NC) {
        pinmap_pinout(init->mclk, PinMap_SAI_MCLK);
    }
    pinmap_pinout(init->sd, PinMap_SAI_SD);
    pinmap_pinout(init->bclk, PinMap_SAI_BCLK);
    pinmap_pinout(init->wclk, PinMap_SAI_WCLK);

    /* I2S configuration -------------------------------------------------------*/
    if (LL_I2S_DeInit(obj->base->u.spi) != SUCCESS) {
        return SAI_RESULT_GENERIC_FAILURE;
    }
    
    /* Configure the Audio Frequency, Standard and the data format */
    LL_I2S_InitTypeDef init_struct;
    
    LL_I2S_StructInit(&init_struct);
    init_struct.Mode = LL_I2S_MODE_SLAVE_RX;
    init_struct.Standard = LL_I2S_STANDARD_PHILIPS;
    init_struct.DataFormat = LL_I2S_DATAFORMAT_16B_EXTENDED;
    init_struct.MCLKOutput = LL_I2S_MCLK_OUTPUT_ENABLE;
    init_struct.AudioFreq = LL_I2S_AUDIOFREQ_16K;
    init_struct.ClockPolarity = LL_I2S_POLARITY_LOW;
    
    if (LL_I2S_Init(obj->base->u.spi, &init_struct) != SUCCESS) {
        return SAI_RESULT_GENERIC_FAILURE;
    }

    LL_I2S_Enable(obj->base->u.spi);

    return SAI_RESULT_OK;
}
static bool stm_i2s_xfer(sai_t *obj, uint32_t *psample) {
    bool ret = false;
    if (obj->is_receiver) {
        /* Data Received through I2Sx SD pin */ 
        ret = LL_SPI_IsActiveFlag_RXNE(obj->base->u.spi);
        if (ret) {
            uint32_t sample = LL_I2S_ReceiveData16(obj->base->u.spi);
            if (psample != NULL) {
                *psample = sample;
            }
        }
    } else {
        uint32_t sample = 0;
        if (psample != NULL) { sample = *psample; }
        
        /* Data to transmitted through I2Sx_ext SD pin */
        ret = LL_SPI_IsActiveFlag_TXE(obj->base->u.spi);
        if (ret) {
            LL_I2S_TransmitData16(obj->base->u.spi, sample);
        }
    }
    return ret;
}
static void stm_i2s_free(sai_t *obj) {
    while (LL_I2S_DeInit(obj->base->u.spi) != SUCCESS) ;
}

#endif // DEVICE_SAI
