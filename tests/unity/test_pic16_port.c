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
 * @file test_pic16_port.c
 * @brief GPIO isolation and real port GIE/latch ordering on every supported pin.
 */
#include "ws2812_port.h"

#include <string.h>

#include "unity.h"
#include "xc.h"

FakeIntconBits INTCONbits;
uint8_t LATA, LATC, TRISA, TRISC, ANSELA, ANSELC;
uint8_t PWM1CON, PWM2CON, FSR0L, FSR0H, WREG;

static unsigned int emits, latch_calls, startup_calls;

static bool expected_gie;

static const volatile uint8_t *expected_data;

static uint8_t expected_count;

static const char *expected_gpio;

void setUp(void) {
    emits = latch_calls = startup_calls = 0;
    expected_gie = INTCONbits.GIE = true;
    LATA = LATC = TRISA = TRISC = ANSELA = ANSELC = PWM1CON = PWM2CON = 255;
    FSR0L = FSR0H = WREG = 0;
}

void tearDown(void) {
}

void FakeHardwareEmit(const char *instructions) {
    TEST_ASSERT_NOT_NULL(strstr(instructions, expected_gpio));
    TEST_ASSERT_FALSE(INTCONbits.GIE);
    TEST_ASSERT_EQUAL_UINT8((uintptr_t)expected_data, FSR0L);
    TEST_ASSERT_EQUAL_UINT8((uintptr_t)expected_data >> 8, FSR0H);
    TEST_ASSERT_EQUAL_UINT8(expected_count, WREG);
    ++emits;
}

void FakeHardwareDelayMs(unsigned int duration) {
    TEST_ASSERT_EQUAL_UINT(1, duration);
    TEST_ASSERT_EQUAL(expected_gie, INTCONbits.GIE);
    ++startup_calls;
}

void FakeHardwareDelayUs(unsigned int duration) {
    TEST_ASSERT_EQUAL_UINT(300, duration);
    TEST_ASSERT_EQUAL(expected_gie, INTCONbits.GIE);
    TEST_ASSERT_EQUAL_UINT(latch_calls + 1, emits);
    ++latch_calls;
}

static void TestInitChangesOnlySelectedPinAndApplicablePeripheral(void) {
    // Order follows all_pins.h; RA pins leave the whole C port untouched.
    const uint8_t masks[] = {32, 4, 1, 2, 8, 16, 16, 32};
    for (unsigned int i = 0; i < kWs2812ChainCount; ++i) {
        setUp();
        expected_gie = INTCONbits.GIE = (i & 1u) != 0;
        Ws2812PortInit((Ws2812Chain)i);
        uint8_t mask = masks[i];
        bool port_a = i >= 6;
        TEST_ASSERT_EQUAL_HEX8(port_a ? (uint8_t)~mask : 255, LATA);
        TEST_ASSERT_EQUAL_HEX8(port_a ? (uint8_t)~mask : 255, TRISA);
        TEST_ASSERT_EQUAL_HEX8(port_a ? 255 : (uint8_t)~mask, LATC);
        TEST_ASSERT_EQUAL_HEX8(port_a ? 255 : (uint8_t)~mask, TRISC);
        TEST_ASSERT_EQUAL_HEX8(!port_a && mask <= 8 ? (uint8_t)~mask : 255, ANSELC);
        TEST_ASSERT_EQUAL_HEX8(i == 0 ? 0 : 255, PWM1CON);
#if defined(_16F1459) || defined(_16LF1459)
        TEST_ASSERT_EQUAL_HEX8(255, PWM2CON);
#else
        TEST_ASSERT_EQUAL_HEX8(i == 4 ? 0 : 255, PWM2CON);
#endif
#if defined(_16F1455) || defined(_16LF1455) || defined(_16F1459) || defined(_16LF1459)
        TEST_ASSERT_EQUAL_HEX8(i == 6 ? 0xef : 255, ANSELA);
#else
        TEST_ASSERT_EQUAL_HEX8(255, ANSELA);
#endif
        TEST_ASSERT_EQUAL_UINT(1, startup_calls);
        TEST_ASSERT_EQUAL_UINT(0, emits);
    }
}

static void CheckWrite(bool entry_gie) {
    const uint8_t bytes[3] = {0x12, 0xab, 0x55};
    const char *gpio[] = {
        "bsf (LATC & 0x7f),5\n", "bsf (LATC & 0x7f),2\n",
        "bsf (LATC & 0x7f),0\n", "bsf (LATC & 0x7f),1\n",
        "bsf (LATC & 0x7f),3\n", "bsf (LATC & 0x7f),4\n",
        "bsf (LATA & 0x7f),4\n", "bsf (LATA & 0x7f),5\n"
    };
    for (unsigned int i = 0; i < kWs2812ChainCount; ++i) {
        expected_gpio = gpio[i];
        expected_data = bytes;
        expected_count = 3;
        expected_gie = INTCONbits.GIE = entry_gie;
        Ws2812PortWrite((Ws2812Chain)i, bytes, 3);
        TEST_ASSERT_EQUAL_UINT(i + 1, emits);
        TEST_ASSERT_EQUAL_UINT(i + 1, latch_calls);
        TEST_ASSERT_EQUAL(entry_gie, INTCONbits.GIE);
        TEST_ASSERT_EQUAL_HEX8(0x12, bytes[0]);
        TEST_ASSERT_EQUAL_HEX8(0xab, bytes[1]);
        TEST_ASSERT_EQUAL_HEX8(0x55, bytes[2]);
    }
}

static void TestWriteMasksAndRestoresEnabledInterruptsBeforeLatch(void) {
    CheckWrite(true);
}

static void TestWriteKeepsDisabledInterruptsDisabledThroughLatch(void) {
    CheckWrite(false);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(TestInitChangesOnlySelectedPinAndApplicablePeripheral);
    RUN_TEST(TestWriteMasksAndRestoresEnabledInterruptsBeforeLatch);
    RUN_TEST(TestWriteKeepsDisabledInterruptsDisabledThroughLatch);
    return UNITY_END();
}
