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
 * @file test_ws2812.c
 * @brief Public API behavior against a link-time fake hardware backend.
 */
#include "ws2812.h"

#include <stddef.h>

#include "unity.h"
#include "ws2812_port_fake.h"

static const Ws2812Color kBlack = {0, 0, 0};

static const Ws2812Color kColor = {255, 128, 17};

void setUp(void) {
    FakeWs2812PortReset();
    TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainMain, 3));
}

void tearDown(void) {
}

static void AssertColor(Ws2812Color expected, Ws2812Color actual) {
    TEST_ASSERT_EQUAL_UINT8(expected.red, actual.red);
    TEST_ASSERT_EQUAL_UINT8(expected.green, actual.green);
    TEST_ASSERT_EQUAL_UINT8(expected.blue, actual.blue);
}

static void AssertWireColor(uint8_t index, Ws2812Color color) {
    uint8_t offset = (uint8_t)(3u * index);
    TEST_ASSERT_EQUAL_UINT8(color.green, fake_ws2812_port.bytes[offset]);
    TEST_ASSERT_EQUAL_UINT8(color.red, fake_ws2812_port.bytes[offset + 1u]);
    TEST_ASSERT_EQUAL_UINT8(color.blue, fake_ws2812_port.bytes[offset + 2u]);
}

static void TestInitClearsEverySupportedLength(void) {
    for (uint8_t count = 1; count <= WS2812_MAX_LEDS; ++count) {
        TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainMain, count));
        TEST_ASSERT_EQUAL_UINT8(count, Ws2812Count(kWs2812ChainMain));
        TEST_ASSERT_EQUAL_UINT8(3u * count, fake_ws2812_port.byte_count);
        TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
        for (uint8_t i = 0; i < count; ++i) {
            Ws2812Color color;
            TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, i, &color));
            AssertColor(kBlack, color);
            AssertWireColor(i, kBlack);
        }

        TEST_ASSERT_TRUE(Ws2812Fill(kWs2812ChainMain, kColor, 100));
    }
}

static void TestIndexedEditsAreBufferedAndPreserveOtherPixels(void) {
    unsigned int writes = fake_ws2812_port.write_calls;
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 2, kColor, 100));
    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812ShowIfDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
    AssertWireColor(0, kBlack);
    AssertWireColor(1, kBlack);
    AssertWireColor(2, kColor);
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 2, kColor, 100));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812ShowIfDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
}

static void TestEveryIndexAcrossSupportedLengths(void) {
    for (uint8_t count = 1; count <= WS2812_MAX_LEDS; ++count) {
        TEST_ASSERT_TRUE(Ws2812Init(kWs2812ChainMain, count));
        unsigned int writes = fake_ws2812_port.write_calls;
        for (uint8_t index = 0; index < count; ++index) {
            Ws2812Color color = {
                (uint8_t)(index + 1u), (uint8_t)(200u - index), index};
            TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, index, color, 100));
        }

        TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
        TEST_ASSERT_TRUE(Ws2812Show(kWs2812ChainMain));
        TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
        for (uint8_t index = 0; index < count; ++index) {
            Ws2812Color expected = {
                (uint8_t)(index + 1u), (uint8_t)(200u - index), index};
            Ws2812Color actual;
            TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, index, &actual));
            AssertColor(expected, actual);
            AssertWireColor(index, expected);
        }
    }
}

static void TestBrightnessRoundsAndClampsEveryByteInput(void) {
    unsigned int writes = fake_ws2812_port.write_calls;
    for (unsigned int percent = 0; percent <= 255; ++percent) {
        unsigned int brightness = percent > 100 ? 100 : percent;
        for (unsigned int value = 0; value <= 255; ++value) {
            Ws2812Color input = {
                (uint8_t)value, (uint8_t)(255 - value), (uint8_t)(value ^ 0x55)};
            Ws2812Color expected = {
                (uint8_t)((input.red * brightness + 50) / 100),
                (uint8_t)((input.green * brightness + 50) / 100),
                (uint8_t)((input.blue * brightness + 50) / 100)};
            Ws2812Color actual;
            TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 1, input, (uint8_t)percent));
            TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, 1, &actual));
            AssertColor(expected, actual);
        }
    }

    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
}

static void TestBulkEditsCopyBorrowedInputAndFlushOnce(void) {
    Ws2812Color colors[3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
    Ws2812Color red = {77, 0, 0}, green = {0, 77, 0}, blue = {0, 0, 77};
    unsigned int writes = fake_ws2812_port.write_calls;
    TEST_ASSERT_TRUE(Ws2812SetPixels(kWs2812ChainMain, 0, colors, 3, 30));
    colors[0] = kBlack;
    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
    TEST_ASSERT_TRUE(Ws2812Show(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
    AssertWireColor(0, red);
    AssertWireColor(1, green);
    AssertWireColor(2, blue);
}

static void TestInvalidRangesDoNotPartiallyEditOrTransmit(void) {
    Ws2812Color colors[3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    unsigned int writes = fake_ws2812_port.write_calls;
    unsigned int inits = fake_ws2812_port.init_calls;
    TEST_ASSERT_FALSE(Ws2812SetPixel(kWs2812ChainMain, 3, kColor, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixel(kWs2812ChainMain, 255, kColor, 100));
    TEST_ASSERT_FALSE(Ws2812SetColor(kWs2812ChainMain, 3, 255, 128, 17, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(kWs2812ChainMain, 2, colors, 2, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(kWs2812ChainMain, 0, NULL, 1, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(kWs2812ChainMain, 255, colors, 255, 100));
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainMain, 0));
    TEST_ASSERT_FALSE(Ws2812Init(kWs2812ChainMain, WS2812_MAX_LEDS + 1));
    TEST_ASSERT_EQUAL_UINT8(3, Ws2812Count(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
    TEST_ASSERT_EQUAL_UINT(inits, fake_ws2812_port.init_calls);
    for (uint8_t i = 0; i < 3; ++i) {
        Ws2812Color color;
        TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, i, &color));
        AssertColor(kBlack, color);
    }
}

static void TestReadbackErrorsLeaveOutputUntouched(void) {
    Ws2812Color output = {1, 2, 3}, expected = output;
    TEST_ASSERT_FALSE(Ws2812GetPixel(kWs2812ChainMain, 0, NULL));
    TEST_ASSERT_FALSE(Ws2812GetPixel(kWs2812ChainMain, 3, &output));
    AssertColor(expected, output);
    TEST_ASSERT_FALSE(Ws2812GetPixel(kWs2812ChainMain, 255, &output));
    AssertColor(expected, output);
}

static void TestEmptyRangesHaveNoSideEffects(void) {
    unsigned int writes = fake_ws2812_port.write_calls;
    TEST_ASSERT_TRUE(Ws2812SetPixels(kWs2812ChainMain, 3, NULL, 0, 100));
    TEST_ASSERT_TRUE(Ws2812SetPixels(kWs2812ChainMain, 0, NULL, 0, 100));
    TEST_ASSERT_FALSE(Ws2812SetPixels(kWs2812ChainMain, 4, NULL, 0, 100));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
}

static void TestDirtyTrackingAndExplicitResend(void) {
    unsigned int writes = fake_ws2812_port.write_calls;
    TEST_ASSERT_TRUE(Ws2812ShowIfDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes, fake_ws2812_port.write_calls);
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 0, kBlack, 100));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 0, kColor, 100));
    TEST_ASSERT_TRUE(Ws2812SetPixel(kWs2812ChainMain, 0, kBlack, 100));
    TEST_ASSERT_TRUE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812ShowIfDirty(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812Show(kWs2812ChainMain));
    TEST_ASSERT_EQUAL_UINT(writes + 2, fake_ws2812_port.write_calls);
}

static void TestImmediateUpdateFlushesPendingColors(void) {
    unsigned int writes = fake_ws2812_port.write_calls;
    Ws2812Color scaled = {77, 38, 5};
    TEST_ASSERT_TRUE(Ws2812Fill(kWs2812ChainMain, kColor, 30));
    TEST_ASSERT_TRUE(Ws2812SetColor(kWs2812ChainMain, 0, 0, 0, 0, 100));
    TEST_ASSERT_EQUAL_UINT(writes + 1, fake_ws2812_port.write_calls);
    AssertWireColor(0, kBlack);
    AssertWireColor(1, scaled);
    AssertWireColor(2, scaled);
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    TEST_ASSERT_TRUE(Ws2812SetColor(kWs2812ChainMain, 0, 0, 0, 0, 100));
    TEST_ASSERT_EQUAL_UINT(writes + 2, fake_ws2812_port.write_calls);
}

static void TestClearDiscardsPendingEditsAndLatchesBlack(void) {
    TEST_ASSERT_TRUE(Ws2812Fill(kWs2812ChainMain, kColor, 100));
    TEST_ASSERT_TRUE(Ws2812Clear(kWs2812ChainMain));
    TEST_ASSERT_FALSE(Ws2812IsDirty(kWs2812ChainMain));
    for (uint8_t i = 0; i < 3; ++i) {
        Ws2812Color color;
        TEST_ASSERT_TRUE(Ws2812GetPixel(kWs2812ChainMain, i, &color));
        AssertColor(kBlack, color);
        AssertWireColor(i, kBlack);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(TestInitClearsEverySupportedLength);
    RUN_TEST(TestIndexedEditsAreBufferedAndPreserveOtherPixels);
    RUN_TEST(TestEveryIndexAcrossSupportedLengths);
    RUN_TEST(TestBrightnessRoundsAndClampsEveryByteInput);
    RUN_TEST(TestBulkEditsCopyBorrowedInputAndFlushOnce);
    RUN_TEST(TestInvalidRangesDoNotPartiallyEditOrTransmit);
    RUN_TEST(TestReadbackErrorsLeaveOutputUntouched);
    RUN_TEST(TestEmptyRangesHaveNoSideEffects);
    RUN_TEST(TestDirtyTrackingAndExplicitResend);
    RUN_TEST(TestImmediateUpdateFlushesPendingColors);
    RUN_TEST(TestClearDiscardsPendingEditsAndLatchesBlack);
    return UNITY_END();
}
