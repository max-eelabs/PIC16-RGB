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
 * @file waveform_main.c
 * @brief Link every configured transmitter for emitted-waveform checks.
 * @note Build-only fixture; use examples/main.c for physical clock startup.
 */
#include "ws2812.h"

int main(void) {
    for (unsigned int i = 0; i < kWs2812ChainCount; ++i) {
        Ws2812Init((Ws2812Chain)i, 1);
    }

    while (1) {
        // Keep every configured chain linked into this validation artifact.
    }
}
