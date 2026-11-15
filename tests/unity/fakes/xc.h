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
 * @file xc.h
 * @brief Host-only register declarations for testing the real PIC port C code.
 *
 * Only the native port-test target includes this directory before device headers.
 * The assembly invocation is replaced at preprocessing time, without rewriting
 * production files. Its actual pulse sequence is checked against XC8 HEX output.
 */
#ifndef PIC16_RGB_TESTS_UNITY_FAKES_XC_H_
#define PIC16_RGB_TESTS_UNITY_FAKES_XC_H_

#include <stdbool.h>
#include <stdint.h>

#if !defined(_16F1454) && !defined(_16F1455) && !defined(_16F1459) && \
    !defined(_16LF1454) && !defined(_16LF1455) && !defined(_16LF1459)
#define _16F1454 1
#endif

typedef struct {
    bool GIE;
} FakeIntconBits;

extern FakeIntconBits INTCONbits;
extern uint8_t LATA, LATC, TRISA, TRISC, ANSELA, ANSELC;
extern uint8_t PWM1CON, PWM2CON, FSR0L, FSR0H, WREG;

void FakeHardwareEmit(const char *instructions);
void FakeHardwareDelayMs(unsigned int duration);
void FakeHardwareDelayUs(unsigned int duration);

#define asm(instructions) FakeHardwareEmit(instructions)
#define __delay_ms(duration) FakeHardwareDelayMs(duration)
#define __delay_us(duration) FakeHardwareDelayUs(duration)

#endif
