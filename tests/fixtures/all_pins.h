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

// Exercise every supported literal assembly operand on both MCU targets.
#define WS2812_CHAINS(X) \
    X(Main, RC5, 20) X(Aux, RC2, 5) X(C0, RC0, 1) X(C1, RC1, 1) \
    X(C3, RC3, 1) X(C4, RC4, 1) X(A4, RA4, 1) X(A5, RA5, 1)
