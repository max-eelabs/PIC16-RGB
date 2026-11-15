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
 * @file ws2812_port_fake.c
 * @brief Copy frames at the hardware boundary without source transformation.
 */
#include "ws2812_port_fake.h"

#include "unity.h"
#include "ws2812_port.h"

FakeWs2812PortState fake_ws2812_port;

void FakeWs2812PortReset(void) {
    FakeWs2812PortState empty = {0};
    fake_ws2812_port = empty;
}

void Ws2812PortInit(Ws2812Chain chain) {
    fake_ws2812_port.chain = chain;
    ++fake_ws2812_port.init_calls;
}

void Ws2812PortWrite(Ws2812Chain chain, const volatile uint8_t *grb, uint8_t byte_count) {
    fake_ws2812_port.chain = chain;
    TEST_ASSERT_NOT_NULL(grb);
    TEST_ASSERT_GREATER_THAN_UINT8(0, byte_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(sizeof(fake_ws2812_port.bytes), byte_count);
    TEST_ASSERT_EQUAL_UINT8(0, byte_count % 3);
    for (uint8_t i = 0; i < byte_count; ++i) {
        fake_ws2812_port.bytes[i] = grb[i];
    }

    fake_ws2812_port.byte_count = byte_count;
    ++fake_ws2812_port.write_calls;
}
