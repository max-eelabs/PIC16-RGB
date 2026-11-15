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
 * @file test_chains.c
 * @brief Independent state, invalid identifiers and selected-wire routing.
 */
#include "ws2812.h"

#include "unity.h"
#include "ws2812_port_fake.h"

static const Ws2812Color kRed = {255, 0, 0};

static const Ws2812Color kBlue = {0, 0, 255};

void setUp(void) {
    TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainMain, 3));
    TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainAux, 5));
    FakeWs2812PortReset();
}

void tearDown(void) {
}

static void TestBuffersAndDirtyFlagsAreIndependent(void) {
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 2, kRed, 100));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainAux));
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainAux, 4, kBlue, 50));
    TEST_ASSERT_TRUE(Ws2812Show(kWs2812ChainMain));
    TEST_ASSERT_EQUAL(kWs2812ChainMain, fake_ws2812_port.chain);
    TEST_ASSERT_EQUAL_UINT8(9, fake_ws2812_port.byte_count);
    TEST_ASSERT_EQUAL_UINT8(255, fake_ws2812_port.bytes[7]);
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainAux));
    TEST_ASSERT_TRUE(Ws2812ShowIfDirty(kWs2812ChainAux));
    TEST_ASSERT_EQUAL(kWs2812ChainAux, fake_ws2812_port.chain);
    TEST_ASSERT_EQUAL_UINT8(15, fake_ws2812_port.byte_count);
    TEST_ASSERT_EQUAL_UINT8(128, fake_ws2812_port.bytes[14]);
    TEST_ASSERT_EQUAL_UINT(2, fake_ws2812_port.write_calls);
    Ws2812Color color;
    TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, 2, &color));
    TEST_ASSERT_EQUAL_UINT8(255, color.red);
    TEST_ASSERT_EQUAL_UINT8(0, color.blue);
}

static void TestClearAndReinitializationOnlyAffectSelectedChain(void) {
    TEST_ASSERT_TRUE(Ws2812Fill(kWs2812ChainAux, kBlue, 100));
    TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainMain, 1));
    TEST_ASSERT_EQUAL_UINT8(1, Ws2812Count(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT8(5, Ws2812Count(kWs2812ChainAux));
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainAux));
    TEST_ASSERT_TRUE(Ws2812Clear(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainAux));
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainMain, 4));
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainAux, 6));
    TEST_ASSERT_FALSE(Ws2812SetPixel(kWs2812ChainMain, 1, kRed, 100));
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainAux, 4, kRed, 100));
}

static void CheckInvalid(Ws2812Chain invalid) {
    Ws2812Color output = {1, 2, 3};
    TEST_ASSERT_FALSE(Ws2812Init(invalid, 1));
    TEST_ASSERT_EQUAL_UINT8(0, Ws2812Count(invalid));
    TEST_ASSERT_FALSE(Ws2812SetPixel(invalid, 0, kRed, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(invalid, 0, &kRed, 1, 100));
    TEST_ASSERT_FALSE(Ws2812Fill(invalid, kRed, 100));
    TEST_ASSERT_FALSE(Ws2812GetPixel(invalid, 0, &output));
    TEST_ASSERT_EQUAL_UINT8(1, output.red);
    TEST_ASSERT_EQUAL_UINT8(2, output.green);
    TEST_ASSERT_EQUAL_UINT8(3, output.blue);
    TEST_ASSERT_FALSE(Ws2812IsDirty(invalid));
    TEST_ASSERT_FALSE(Ws2812Show(invalid));
    TEST_ASSERT_FALSE(Ws2812ShowIfDirty(invalid));
    TEST_ASSERT_FALSE(Ws2812SetColor(invalid, 0, 1, 2, 3, 100));
    TEST_ASSERT_FALSE(Ws2812Clear(invalid));
}

static void TestInvalidIdsHaveNoSideEffects(void) {
    TEST_ASSERT_TRUE(Ws2812Fill(kWs2812ChainAux, kBlue, 100));
    CheckInvalid((Ws2812Chain)-1);
    CheckInvalid(kWs2812ChainCount);
    CheckInvalid((Ws2812Chain)255);
    TEST_ASSERT_EQUAL_UINT(0, fake_ws2812_port.init_calls);
    TEST_ASSERT_EQUAL_UINT(0, fake_ws2812_port.write_calls);
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainAux));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(TestBuffersAndDirtyFlagsAreIndependent);
    RUN_TEST(TestClearAndReinitializationOnlyAffectSelectedChain);
    RUN_TEST(TestInvalidIdsHaveNoSideEffects);
    return UNITY_END();
}
