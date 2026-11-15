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
 * @file ws2812_pic16.c
 * @brief Build-time GPIO-specialized PIC16(L)F1454/5/9 transmitters at 48 MHz.
 */
#include "ws2812_port.h"

#include <xc.h>

#if !defined(_16F1454) && !defined(_16F1455) && !defined(_16F1459) && \
    !defined(_16LF1454) && !defined(_16LF1455) && !defined(_16LF1459)
#error "This backend supports PIC16(L)F1454/5/9 only"
#endif

// Expand pin tokens before stringifying assembly operands.
#define WS_TEXT_INNER(value) #value
#define WS_TEXT(value) WS_TEXT_INNER(value)
#define WS_LAT_RA4 LATA
#define WS_LAT_RA5 LATA
#define WS_LAT_RC0 LATC
#define WS_LAT_RC1 LATC
#define WS_LAT_RC2 LATC
#define WS_LAT_RC3 LATC
#define WS_LAT_RC4 LATC
#define WS_LAT_RC5 LATC
#define WS_BIT_RA4 4
#define WS_BIT_RA5 5
#define WS_BIT_RC0 0
#define WS_BIT_RC1 1
#define WS_BIT_RC2 2
#define WS_BIT_RC3 3
#define WS_BIT_RC4 4
#define WS_BIT_RC5 5
#define WS_PAD3 "nop\n" "nop\n" "nop\n"
#define WS_PAD4 WS_PAD3 "nop\n"
#define WS_HIGH(pin) "bsf (" WS_TEXT(WS_LAT_##pin) " & 0x7f)," WS_TEXT(WS_BIT_##pin) "\n"
#define WS_LOW(pin) "bcf (" WS_TEXT(WS_LAT_##pin) " & 0x7f)," WS_TEXT(WS_BIT_##pin) "\n"
#define WS_BIT(pin, bit, padding) \
    WS_HIGH(pin) "nop\n" "nop\n" \
    "btfss INDF0," #bit "\n" WS_LOW(pin) \
    "nop\n" "nop\n" "nop\n" WS_LOW(pin) \
    padding "btfsc INDF0," #bit "\n" "bra $+1\n"
#define WS_BYTE_PREFIX(pin) \
    WS_BIT(pin, 7, WS_PAD4) WS_BIT(pin, 6, WS_PAD4) WS_BIT(pin, 5, WS_PAD4) \
    WS_BIT(pin, 4, WS_PAD4) WS_BIT(pin, 3, WS_PAD4) WS_BIT(pin, 2, WS_PAD4) WS_BIT(pin, 1, WS_PAD4)

// Dispatch happens before GIE masking. Only literal pin operands occur in the
// timed loop. INDF0 data reads allow BSR to stay on the selected LAT bank.
// ADDFSR + DECFSZ + BRA replace the last bit's four padding cycles.
#define WS_TRANSMITTER(name, pin, capacity) \
    static void Write##name(const volatile uint8_t *grb, uint8_t byte_count) { \
        bool interrupts_enabled = INTCONbits.GIE; \
        INTCONbits.GIE = 0; \
        FSR0L = (uint8_t)(uintptr_t)grb; \
        FSR0H = (uint8_t)((uintptr_t)grb >> 8); \
        WREG = byte_count; \
        asm("banksel " WS_TEXT(WS_LAT_##pin) "\n" \
            "ws2812_byte_" #name ":\n" \
            WS_BYTE_PREFIX(pin) WS_BIT(pin, 0, "") \
            "addfsr 0,1\n" "decfsz WREG,f\n" \
            "bra ws2812_byte_" #name "\n"); \
        INTCONbits.GIE = interrupts_enabled; \
        __delay_us(300); \
    }
WS2812_CHAINS(WS_TRANSMITTER)
#undef WS_TRANSMITTER

// RA4 has an analog selector on 1455/1459. The 1454 has no analog input.
#if defined(_16F1455) || defined(_16LF1455) || defined(_16F1459) || defined(_16LF1459)
#define WS_RA4_DIGITAL ANSELA &= (uint8_t)~0x10u;
#else
#define WS_RA4_DIGITAL
#endif

// On the 1459, PWM2 belongs to RC6; claiming RC3 must leave that output alone.
#if defined(_16F1459) || defined(_16LF1459)
#define WS_RC3_RELEASE_PWM
#else
#define WS_RC3_RELEASE_PWM PWM2CON = 0;
#endif

// Only selected GPIO/peripheral bits change. Oscillator and other alternate
// peripheral pin ownership remain the application's responsibility.
#define WS_INIT_RA4 WS_RA4_DIGITAL LATA &= (uint8_t)~0x10u; TRISA &= (uint8_t)~0x10u
#define WS_INIT_RA5 LATA &= (uint8_t)~0x20u; TRISA &= (uint8_t)~0x20u
#define WS_INIT_RC0 ANSELC &= (uint8_t)~1u; LATC &= (uint8_t)~1u; TRISC &= (uint8_t)~1u
#define WS_INIT_RC1 ANSELC &= (uint8_t)~2u; LATC &= (uint8_t)~2u; TRISC &= (uint8_t)~2u
#define WS_INIT_RC2 ANSELC &= (uint8_t)~4u; LATC &= (uint8_t)~4u; TRISC &= (uint8_t)~4u
#define WS_INIT_RC3 WS_RC3_RELEASE_PWM ANSELC &= (uint8_t)~8u; LATC &= (uint8_t)~8u; TRISC &= (uint8_t)~8u
#define WS_INIT_RC4 LATC &= (uint8_t)~0x10u; TRISC &= (uint8_t)~0x10u
#define WS_INIT_RC5 PWM1CON = 0; LATC &= (uint8_t)~0x20u; TRISC &= (uint8_t)~0x20u

void Ws2812PortInit(Ws2812Chain chain) {
    switch (chain) {
#define WS_INIT_CASE(name, pin, capacity) case kWs2812Chain##name: { WS_INIT_##pin; break; }
        WS2812_CHAINS(WS_INIT_CASE)
#undef WS_INIT_CASE
        default: return;
    }

    __delay_ms(1);
}

void Ws2812PortWrite(Ws2812Chain chain, const volatile uint8_t *grb,
                     uint8_t byte_count) {
    switch (chain) {
#define WS_WRITE_CASE(name, pin, capacity) case kWs2812Chain##name: Write##name(grb, byte_count); return;
        WS2812_CHAINS(WS_WRITE_CASE)
#undef WS_WRITE_CASE
        default: return;
    }
}
