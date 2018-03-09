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
/**
 * \defgroup hal_sai Serial audio interface hal functions
 * Low level interface to the serial audio interface of a target.
 *
 * # Defined behavior
 * * Supports a subset of the possible configuration space - verified by ::sai_
 * * Reports a failure (returns false) upon any invocation to an unsupported feature/parameter - verified by ::sai_
 * * Is able to indicate the currently processed word - verified by ::sai_
 * # Defined behavior if feature supported
 * * Is able to change receiver format without interrupting transmitter format - verified by ::sai_
 * * Is able to change transmitter format without interrupting receiver format - verified by ::sai_
 * * Is able to change transmitter and receiver format without interrupting the current frame - verified by ::sai_
 *
 * @note
 * A transceiver supporting async rx/tx should be considered as 2 different peripherals :
 * - one read-only
 * - one write-only
 * The first allocated channel may or may not limit the second one's feature.
 * eg:
 * In a peripheral that supports async rx/tx but requires format to be the same,
 * the first allocated instance will set the format and tie the second one to this format.
 *
 * @{
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

  sai_format_t  format;
} sai_channel_init_t;

/** Init structure */
typedef struct sai_init_s {
  sai_synchronicity_t sync; // rx/tx independant(aka async), synced to rx, synced to tx

  PinName       mclk;
  bool          mclk_internal_src;
  uint32_t      mclk_freq;
  sai_channel_init_t TX;
  sai_channel_init_t RX;
} sai_init_t;

/** Delegated typedef @see target/ ** /object.h */
typedef struct sai_s sai_t;

/** SAI configuration for I2S Philips 16 bit data & word size */
extern const sai_format_t sai_mode_i2s16;
/** SAI configuration for I2S Philips 32 bit data & word size */
extern const sai_format_t sai_mode_i2s32;
/** SAI configuration for PCM 16 bit data & word size with long sync */
extern const sai_format_t sai_mode_pcm16l;
/** SAI configuration for PCM 16 bit data & word size with short sync */
extern const sai_format_t sai_mode_pcm16s;

/** Initialize `obj` based on `init` values.
 * This function may fail if the underlying peripheral does not support the requested features.
 * @return `false` on failure.
 */
bool sai_init(sai_t *obj, sai_init_t *init);

/** Transfer a sample and return the sample received meanwhile. */
uint32_t sai_xfer(sai_t *obj, uint32_t channel, uint32_t sample);

/** Changes the format of the receiver channel only */
bool sai_configure_receiver_format(sai_t *obj, sai_format_t *fmt);

/** Changes the format of the transmitter channel only */
bool sai_configure_transmitter_format(sai_t *obj, sai_format_t *fmt);

/** Changes the format of both transmitter & receiver */
bool sai_configure_format(sai_t *obj, sai_format_t *fmt);

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
