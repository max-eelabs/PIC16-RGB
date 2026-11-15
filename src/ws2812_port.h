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
 * @file ws2812_port.h
 * @brief Private synchronous hardware boundary for independent chains.
 */
#ifndef PIC16_RGB_SRC_WS2812_PORT_H_
#define PIC16_RGB_SRC_WS2812_PORT_H_

#include "ws2812.h"

/**
 * @brief Claim the selected digital GPIO, drive low, and wait 1 ms.
 * @note Caller supplies a valid chain, stable 48 MHz clock and main context.
 * Preserves GIE and unrelated pins. Releases PWM only on selected RC3/RC5.
 * The application must release other peripherals sharing the selected pin.
 */
void Ws2812PortInit(Ws2812Chain chain);

/**
 * @brief Send a validated, borrowed GRB buffer on the selected chain only.
 * @param chain Valid configured chain; dispatch occurs outside the timed loop.
 * @param grb Contiguous storage valid through return; no pointer is retained.
 * @param byte_count Nonzero multiple of three, at most 60 bytes.
 * @note Main context only, non-reentrant across all chains, 48 MHz. Masks GIE
 * during transmission, restores entry state before the 300 us latch, and leaves
 * DIN low. No electrical acknowledgement exists. Does not modify data.
 */
void Ws2812PortWrite(Ws2812Chain chain, const volatile uint8_t *grb,
                     uint8_t byte_count);

#endif
