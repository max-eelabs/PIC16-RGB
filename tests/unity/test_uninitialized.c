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
 * @file test_uninitialized.c
 * @brief Initial-state tests run in a separate process from initialized cases.
 */
#include "ws2812.h"

#include <stddef.h>

#include "unity.h"
#include "ws2812_port_fake.h"

void setUp(void) {
    FakeWs2812PortReset();
}

void tearDown(void) {
}

static void TestCallsBeforeInitHaveNoSideEffects(void) {
    Ws2812Color input = {255, 128, 17};
    Ws2812Color output = {1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8(0, Ws2812Count(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812Show(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812ShowIfDirty(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812Clear(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812Fill(kWs2812ChainMain, input, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixel(kWs2812ChainMain, 0, input, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(kWs2812ChainMain, 0, NULL, 0, 100));
    TEST_ASSERT_FALSE(Ws2812SetColor(kWs2812ChainMain, 0, 255, 128, 17, 30));
    TEST_ASSERT_FALSE(Ws2812GetPixel(kWs2812ChainMain, 0, &output));
    TEST_ASSERT_EQUAL_UINT8(1, output.red);
    TEST_ASSERT_EQUAL_UINT8(2, output.green);
    TEST_ASSERT_EQUAL_UINT8(3, output.blue);
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainMain, 0));
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainMain, WS2812_MAX_LEDS + 1));
    TEST_ASSERT_EQUAL_UINT(0, fake_ws2812_port.init_calls);
    TEST_ASSERT_EQUAL_UINT(0, fake_ws2812_port.write_calls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(TestCallsBeforeInitHaveNoSideEffects);
    return UNITY_END();
}
