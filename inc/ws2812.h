/*
 * Copyright 2026 Maxim Pavlov
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ws2812.h
 * @brief Independent WS2812B-V6 GPIO chains on PIC16(L)F1454/5/9 at 48 MHz.
 *
 * Each configured chain owns its GPIO, static buffer, length and dirty flag.
 * All calls are main-context only and non-reentrant, including across chains.
 * Every operation affects only its selected chain. Shows run sequentially;
 * separate chains do not latch simultaneously. Invalid chain IDs have no effects.
 * Buffered operations do not touch the wire. Show operations block until the
 * complete frame and latch finish. No caller pointers are retained.
 */
#ifndef PIC16_RGB_WS2812_H_
#define PIC16_RGB_WS2812_H_

#include <stdbool.h>
#include <stdint.h>

#include "ws2812_config.h"

// Delay macros and assembly require the actual CPU oscillator to be 48 MHz.
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 48000000UL
#elif _XTAL_FREQ != 48000000UL
#error "This WS2812 transmitter requires a 48 MHz CPU clock"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// IDs are generated from the shared build-time chain table.
#define WS2812_CHAIN_ID(name, pin, capacity) kWs2812Chain##name,
typedef enum {
    WS2812_CHAINS(WS2812_CHAIN_ID)
    kWs2812ChainCount
} Ws2812Chain;
#undef WS2812_CHAIN_ID

// These names deliberately reject unsupported pins and repeated pin ownership.
enum {
    kWs2812PinRA4 = 1, kWs2812PinRA5 = 1,
    kWs2812PinRC0 = 1, kWs2812PinRC1 = 1, kWs2812PinRC2 = 1,
    kWs2812PinRC3 = 1, kWs2812PinRC4 = 1, kWs2812PinRC5 = 1
};
#define WS2812_VALIDATE(name, pin, capacity) \
    typedef char Ws2812Capacity##name[((capacity) >= 1 && (capacity) <= 20) ? 1 : -1]; \
    enum { kWs2812Owns##pin = kWs2812Pin##pin };
WS2812_CHAINS(WS2812_VALIDATE)
#undef WS2812_VALIDATE
typedef char Ws2812NonemptyConfiguration[(kWs2812ChainCount > 0) ? 1 : -1];

// RGB channel values before assignment scaling, or effective values on readback.
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Ws2812Color;

/**
 * @brief Configure the selected GPIO and synchronously latch black on led_count LEDs.
 * @param chain Build-time chain identifier; required on every API call.
 * @param led_count Active chain length, 1 through this chain's configured capacity.
 * @return True on success. Invalid counts leave hardware and state unchanged.
 * @note Call in main context after setting a stable 48 MHz CPU clock. Disables
 * PWM1 on RC5, or PWM2 on RC3 for 1454/1455 only; drives the GPIO low for 1 ms,
 * then clears that chain. Does not configure the
 * oscillator, voltage supply, USB, Timer2, or other peripheral ownership.
 * The caller must release other peripheral functions on the selected GPIO
 * and ensure oscillator/configuration settings allow GPIO use (RA4/RA5).
 * Clears only the selected analog bit and preserves unrelated register bits.
 * Reinitializing clears the new active length, not LEDs beyond that length.
 * Clear the old chain first before shortening it if trailing LEDs must go off.
 */
bool Ws2812Init(Ws2812Chain chain, uint8_t led_count);

/**
 * @brief Return the initialized chain length, or zero for an invalid/uninitialized
 * chain.
 */
uint8_t Ws2812Count(Ws2812Chain chain);

/**
 * @brief Store a scaled RGB pixel without transmitting.
 * @param led_index Zero-based position; zero is nearest the PIC.
 * @param color RGB values, each 0-255, copied into driver-owned GRB storage.
 * @param brightness_percent Linear assignment scaling, clamped to 100.
 * @return False for an invalid chain/index or before initialization; no side effects.
 * @note Scaling rounds: (channel * min(percent, 100) + 50) / 100. This is not
 * gamma correction. Original unscaled values are not retained. A changed
 * effective color marks the frame dirty; a repeated identical color does not.
 */
bool Ws2812SetPixel(Ws2812Chain chain, uint8_t led_index, Ws2812Color color,
                    uint8_t brightness_percent);

/**
 * @brief Copy and scale a contiguous range without transmitting.
 * @param first_index First destination index, zero-based.
 * @param colors Array of at least count RGB values, borrowed during this call.
 * @param count Number of colors to copy; zero is a successful no-op when
 * initialized and first_index is at most the chain length; colors may be null.
 * @param brightness_percent Per-assignment scaling, clamped to 100.
 * @return False for an invalid chain/range, an uninitialized chain, or a null
 * nonempty array. Invalid calls do not partially modify colors or dirty state.
 */
bool Ws2812SetPixels(Ws2812Chain chain, uint8_t first_index, const Ws2812Color *colors,
                     uint8_t count, uint8_t brightness_percent);

/**
 * @brief Store one scaled color across the active chain without transmitting.
 * @return False for an invalid chain or before initialization; otherwise true.
 * @note Same scaling and dirty-state semantics as Ws2812SetPixel().
 */
bool Ws2812Fill(Ws2812Chain chain, Ws2812Color color, uint8_t brightness_percent);

/**
 * @brief Copy the stored effective RGB value into caller-owned output.
 * @return False for an invalid chain/index, an uninitialized chain, or a null
 * output; the output remains unchanged. Otherwise true. No transmission occurs.
 */
bool Ws2812GetPixel(Ws2812Chain chain, uint8_t led_index, Ws2812Color *color);

/**
 * @brief Report whether staged effective values changed since the last Show.
 * @return False for an invalid/uninitialized chain or after a successful Show.
 * @note Dirty is conservative: changing a pixel and changing it back still
 * requires a Show on that chain to clear dirty state. Physical state cannot be read.
 */
bool Ws2812IsDirty(Ws2812Chain chain);

/**
 * @brief Transmit all stored colors, even when the frame is clean.
 * @return False for an invalid chain or before initialization; otherwise true after
 * local transmission.
 * There is no LED acknowledgement or electrical fault detection.
 * @note Main context only, at 48 MHz. Masks GIE for about 30-32 us per LED
 * plus setup, restores the entry GIE state, then waits 300 us with DIN low.
 * Interrupt handlers may extend the latch, but must not touch driver-owned GPIOs
 * or reenter this driver. Owns FSR0/WREG during transmission. Leaves DIN low and clears
 * dirty state. No DMA, timer, SPI, or PWM peripheral generates the waveform.
 */
bool Ws2812Show(Ws2812Chain chain);

/**
 * @brief Show only if dirty; avoid redundant complete-frame transmissions.
 * @return False for an invalid/uninitialized chain; true for a clean no-op or a
 * sent frame. Use Ws2812IsDirty() before this call to distinguish the two cases.
 */
bool Ws2812ShowIfDirty(Ws2812Chain chain);

/**
 * @brief Convenience assignment followed by an unconditional complete Show.
 * @return False for an invalid chain/index or before initialization, with no
 * changes; otherwise true. Other stored pixels are retained and also transmitted.
 * @note Uses the same scaling as SetPixel. Prefer SetPixel/SetPixels plus Show
 * for multiple changes that should appear in a single latched frame.
 */
bool Ws2812SetColor(Ws2812Chain chain, uint8_t led_index, uint8_t red, uint8_t green, uint8_t blue,
                    uint8_t brightness_percent);

/**
 * @brief Clear the whole active chain and immediately Show, even if clean.
 * @return False for an invalid/uninitialized chain; otherwise true after black latches.
 * @note Use before CPU sleep or shortening a chain. This clears every buffered
 * color, including pending edits. Fill with black is the buffered equivalent.
 */
bool Ws2812Clear(Ws2812Chain chain);

#ifdef __cplusplus
}
#endif

#endif
