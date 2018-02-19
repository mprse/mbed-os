/** \addtogroup hal */
/** @{*/
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
#ifndef MBED_SAI_API_H
#define MBED_SAI_API_H

#include "device.h"

#if DEVICE_SAI

#ifdef __cplusplus
extern "C" {
#endif
/** \defgroup hal_sai Serial audio interface hal functions
 * @{
 * @note Implementations may not support all options,
 * In such cases the call should return `False` to report a failure.
 */

/**
 * Used to define how transmitter & receiver are related.
 */
typedef enum sai_synchronicity_e {
    sai_synchronicity_async,
    sai_synchronicity_synced_to_rx,
    sai_synchronicity_synced_to_tx
} sai_synchronicity_t;

/**
 *
 */
typedef struct sai_channel_init_s {
  bool          enable;

  PinName       sd;
  PinName       bclk;
  PinName       wclk;
  bool          is_slave; /**< true if it is slave */
  bool          internal_bclk; /**< true to use internal bclock */
  bool          internal_wclk; /**< true to use internal wclock */

  sai_format_t  fmt;
} sai_channel_init_t;

/**
 * Used to define communication specs.
 */
typedef struct sai_format_s {
  uint32_t  sample_rate; /**< Typically 44100Hz */

  bool      bclk_polarity; /**< true for Active high */
  bool      wclk_polarity; /**< true for Active high */
  bool      ws_delay; /**< true to toggle ws one bit earlier than the frame */
  uint8_t   ws_length; /**< ws assertion length from 1bclk to word length. */

  uint8_t   frame_length; /**< Frame length in word count [1; 32] */
  uint32_t  word_mask; /**< Mask on frame for used word (bitfield) */
  uint8_t   word_length; /**< Word length in bits [1; 32] */
  uint8_t   data_length; /**< Data length within the word. This must <= to word_length. */
  bool      lsb_first; /**< true to send lsb first */
  bool      aligned_left; /**< true to align Left */
  uint8_t   bit_shift; /**< sample bit shift distance from its alignment point. */
} sai_format_t;

/** Init structure */
typedef struct sai_init_s {
  sai_synchronicity_e sync; // rx/tx independant(aka async), synced to rx, synced to tx

  sai_channel_init_t TX;
  sai_channel_init_t RX;
} sai_init_t;

/** Delegated typedef @see target/ ** /object.h */
typedef struct sai_s sai_t;

/** Initialize `obj` based on `init` values.
 * This function may fail if the underlying peripheral does not support the requested features.
 * @return `false` on failure.
 */
bool sai_init(sai_t *obj, sai_init_t *init);

/** Transfer a sample and return the sample received meanwhile. */
uint32_t sai_xfer(sai_t *obj, uint32_t sample);

/** Changes the format of the receiver channel */
bool sai_configure_receiver_format(sai_format_t *fmt);

/** Changes the format of the transmitter channel */
bool sai_configure_transmitter_format(sai_format_t *fmt);

/** Get the current word selection. */
bool sai_get_word_selection(sai_t *obj);

/** Releases & de-initialize the referenced peripherals. */
void sai_free(sai_t *obj);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

/** @}*/
