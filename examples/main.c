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
 * @file main.c
 * @brief Minimal three-LED example; no USB or input processing is configured.
 */
#include "ws2812.h"

#include <xc.h>

#pragma config FOSC = INTOSC, WDTE = OFF, PWRTE = ON, MCLRE = ON
#pragma config CP = OFF, BOREN = ON, CLKOUTEN = OFF, IESO = OFF, FCMEN = OFF
#pragma config WRT = OFF, CPUDIV = NOCLKDIV, USBLSCLK = 48MHz
#pragma config PLLMULT = 3x, PLLEN = ENABLED, STVREN = ON
#pragma config BORV = LO, LPBOR = OFF, LVP = ON

/**
 * @brief Establish 48 MHz, display RGB at 30%, and leave the colors latched.
 * @return Does not return; initialization failure holds the CPU in a loop.
 * @note Requires three externally powered WS2812B-V6 LEDs on RC5.
 */
int main(void) {
    OSCCONbits.IRCF = 0x0f;
    OSCCONbits.SCS = 0;
    OSCCONbits.SPLLMULT = OSCCONbits.SPLLEN = 1;
    while (!OSCSTATbits.PLLRDY) {
        // Timing-sensitive output must wait for PLL lock.
    }

    ANSELA = 0;
    ANSELC = 0;
    if (!Ws2812Init(kWs2812ChainMain, 3)) {
        while (1) {
            // No valid chain is available; do not enter the rendering loop.
        }
    }

    const Ws2812Color colors[3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
    Ws2812SetPixels(kWs2812ChainMain, 0, colors, 3, 30);
    Ws2812Show(kWs2812ChainMain);

    while (1) {
        // LEDs retain colors; the application can service its other work here.
    }
}
