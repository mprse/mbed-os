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

/** Defines frame format. */
typedef struct sai_format_s {
  bool          bclk_polarity;  /**< true for Active high */
  bool          wclk_polarity;  /**< true for Active high */
  bool          ws_delay;       /**< true to toggle ws one bit earlier than the frame */
  uint8_t       ws_length;      /**< ws assertion length from 1bclk to word length. */

  uint8_t       frame_length;   /**< Frame length in word count [1; 32] */
  uint32_t      word_mask;      /**< Mask on frame for used word (bitfield) */
  uint8_t       word_length;    /**< Word length in bits [1; 32] */
  uint8_t       data_length;    /**< Data length within the word. This must <= to word_length. */
  bool          lsb_first;      /**< true to send lsb first */
  bool          aligned_left;   /**< true to align Left */
  uint8_t       bit_shift;      /**< sample bit shift distance from its alignment point. */
} sai_format_t;

/** Init structure */
typedef struct sai_init_s {
  bool          is_synced;      /**< When platform is capable of RX&TX simulaneously,
                                     one's clocks can be tied to the other one.
                                     true if this endpoint it following clock from its counterpart. */
  bool          is_receiver;    /**< true if this is a receiver. */
  bool          is_slave;       /**< true if clocks are inputs. */

  PinName       mclk;           /**< master clock pin (opional, can be NC). */
  PinName       sd;             /**< Data pin. */
  PinName       bclk;           /**< Bit clock pin. */
  PinName       wclk;           /**< Word clock pin. */

  bool          mclk_internal_src; /**< Set true to use internal master clock source. */
  uint32_t      mclk_freq;      /**< Set the master clock frequency. */
  uint32_t      sample_rate;    /**< for example: 44100Hz */
  uint32_t      master_clock;   /**< MCLK frequency */

  sai_format_t  format;         /**< Describes the frame format. */
} sai_init_t;

/** Initialization return status.*/
typedef enum sai_result_e {
    SAI_RESULT_OK,                  /**< Everything went well. */
    SAI_RESULT_DEVICE_BUSY,         /**< All endpoint for the requested type are busy. */
    SAI_RESULT_CONFIG_UNSUPPORTED,  /**< Some requested features are not supported by this platform. */
    SAI_RESULT_CONFIG_MISMATCH,     /**< Some requested features mismatch with the required config
                                         (depends on underlying device state) */
} sai_result_t;

/** Delegated typedef @see target/ ** /object.h */
typedef struct sai_s sai_t;

/** SAI configuration for I2S Philips 16 bit data & word size */
extern const sai_format_t sai_mode_i2s16;
/** SAI configuration for I2S Philips 16bit data & 32bit word size */
extern const sai_format_t sai_mode_i2s16w32;
/** SAI configuration for I2S Philips 32 bit data & word size */
extern const sai_format_t sai_mode_i2s32;
/** SAI configuration for PCM 16 bit data & word size with long sync */
extern const sai_format_t sai_mode_pcm16l;
/** SAI configuration for PCM 16 bit data & word size with short sync */
extern const sai_format_t sai_mode_pcm16s;

/** Initialize `obj` based on `init` values.
 * @return initialization result.
 */
sai_result_t sai_init(sai_t *obj, sai_init_t *init);

/**
 * Attempt to transmit a sample.
 * @param   obj     This SAI instance.
 * @param   sample  Sample to send.
 * @return true if the sample was succesfully pushed to the bus (or the internal fifo).
 */
bool sai_write(sai_t *obj, uint32_t sample);

/**
 * Receive a sample.
 * @param obj       This SAI instance.
 * @param sample    Pointer to store the received sample.
 * @return false if no sample was received or if "sample" is NULL.
 */
bool sai_read(sai_t *obj, uint32_t *sample);

/**
 * Attempt to transmit or receive a sample depending of the mode of this instance.
 * @param obj       This SAI instance.
 * @param sample    Pointer to store or fetch a sample.
 * @return true if the/a sample was transmitted/received.
 */
bool sai_xfer(sai_t *obj, uint32_t *sample);

/** Releases & de-initialize the referenced peripherals. */
void sai_free(sai_t *obj);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

/** @}*/
