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
 * @file ws2812_port_fake.h
 * @brief Observable test state for the private hardware boundary.
 */
#ifndef PIC16_RGB_TESTS_UNITY_FAKES_WS2812_PORT_FAKE_H_
#define PIC16_RGB_TESTS_UNITY_FAKES_WS2812_PORT_FAKE_H_

#include "ws2812.h"

typedef struct {
    uint8_t bytes[3 * WS2812_MAX_LEDS];
    uint8_t byte_count;
    Ws2812Chain chain;
    unsigned int init_calls;
    unsigned int write_calls;
} FakeWs2812PortState;

// Test-owned wire snapshots; core storage is never exposed to tests.
extern FakeWs2812PortState fake_ws2812_port;

/**
 * @brief Reset only fake observations, without changing driver state.
 */
void FakeWs2812PortReset(void);

#endif
