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
 * @file ws2812_config.h
 * @brief Build-time chain declarations shared by all translation units.
 * @note Define WS2812_CONFIG_HEADER to a quoted application header to override
 * WS2812_CHAINS(X). Each row is X(Name, Pin, Capacity). No heap is used.
 */
#ifndef PIC16_RGB_WS2812_CONFIG_H_
#define PIC16_RGB_WS2812_CONFIG_H_

#ifndef WS2812_MAX_LEDS
#define WS2812_MAX_LEDS 20
#endif

#ifdef WS2812_CONFIG_HEADER
#include WS2812_CONFIG_HEADER
#endif

#ifndef WS2812_CHAINS
#define WS2812_CHAINS(X) X(Main, RC5, WS2812_MAX_LEDS)
#endif

#endif
