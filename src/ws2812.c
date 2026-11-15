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
 * @file ws2812.c
 * @brief Portable indexed color storage and synchronous frame API.
 */
#include "ws2812.h"

#include <stddef.h>

#include "ws2812_port.h"

// Each buffer is sized independently; volatile preserves assembly reads.
#define WS2812_BUFFER(name, pin, capacity) static volatile uint8_t grb_##name[3 * (capacity)];
WS2812_CHAINS(WS2812_BUFFER)
#undef WS2812_BUFFER

typedef struct {
    volatile uint8_t *grb;
    uint8_t capacity;
    uint8_t pixel_count;
    bool frame_dirty;
} ChainState;

#define WS2812_STATE(name, pin, capacity) {grb_##name, capacity, 0, false},
static ChainState chains[kWs2812ChainCount] = {
    WS2812_CHAINS(WS2812_STATE)
};
#undef WS2812_STATE

/**
 * @brief Validate an identifier before indexing driver-owned state.
 * @return Selected state, or null for an invalid identifier.
 */
static ChainState *GetChain(Ws2812Chain chain) {
    if ((unsigned int)chain >= (unsigned int)kWs2812ChainCount) {
        return NULL;
    }

    return &chains[chain];
}

/**
 * @brief Scale one channel with rounding; brightness is already clamped.
 * @return Nearest eight-bit linear intensity, without gamma correction.
 */
static uint8_t ScaleChannel(uint8_t channel, uint8_t brightness) {
    return (uint8_t)(((uint16_t)channel * brightness + 50u) / 100u);
}

bool Ws2812Init(Ws2812Chain chain, uint8_t led_count) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (led_count == 0 || led_count > state->capacity) {
        return false;
    }

    state->pixel_count = led_count;
    Ws2812PortInit(chain);

    return Ws2812Clear(chain);
}

uint8_t Ws2812Count(Ws2812Chain chain) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return 0;
    }

    return state->pixel_count;
}

bool Ws2812SetPixel(Ws2812Chain chain, uint8_t led_index, Ws2812Color color,
                    uint8_t brightness_percent) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (led_index >= state->pixel_count) {
        return false;
    }

    if (brightness_percent > 100) {
        brightness_percent = 100;
    }

    uint8_t green = ScaleChannel(color.green, brightness_percent);
    uint8_t red = ScaleChannel(color.red, brightness_percent);
    uint8_t blue = ScaleChannel(color.blue, brightness_percent);
    uint8_t offset = (uint8_t)(3u * led_index);
    if (state->grb[offset] != green || state->grb[offset + 1u] != red ||
            state->grb[offset + 2u] != blue) {
        state->grb[offset] = green;
        state->grb[offset + 1u] = red;
        state->grb[offset + 2u] = blue;
        state->frame_dirty = true;
    }

    return true;
}

bool Ws2812SetPixels(Ws2812Chain chain, uint8_t first_index, const Ws2812Color *colors,
                     uint8_t count, uint8_t brightness_percent) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (state->pixel_count == 0 || first_index > state->pixel_count ||
            count > state->pixel_count - first_index || (count != 0 && colors == NULL)) {
        return false;
    }

    for (uint8_t i = 0; i < count; ++i) {
        Ws2812SetPixel(chain, (uint8_t)(first_index + i), colors[i], brightness_percent);
    }

    return true;
}

bool Ws2812Fill(Ws2812Chain chain, Ws2812Color color, uint8_t brightness_percent) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (state->pixel_count == 0) {
        return false;
    }

    for (uint8_t i = 0; i < state->pixel_count; ++i) {
        Ws2812SetPixel(chain, i, color, brightness_percent);
    }

    return true;
}

bool Ws2812GetPixel(Ws2812Chain chain, uint8_t led_index, Ws2812Color *color) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (led_index >= state->pixel_count || color == NULL) {
        return false;
    }

    uint8_t offset = (uint8_t)(3u * led_index);
    color->red = state->grb[offset + 1u];
    color->green = state->grb[offset];
    color->blue = state->grb[offset + 2u];
    return true;
}

bool Ws2812IsDirty(Ws2812Chain chain) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    return state->frame_dirty;
}

bool Ws2812Show(Ws2812Chain chain) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (state->pixel_count == 0) {
        return false;
    }

    Ws2812PortWrite(chain, state->grb, (uint8_t)(3u * state->pixel_count));
    state->frame_dirty = false;
    return true;
}

bool Ws2812ShowIfDirty(Ws2812Chain chain) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    if (state->pixel_count == 0) {
        return false;
    }

    if (!state->frame_dirty) {
        return true;
    }

    return Ws2812Show(chain);
}

bool Ws2812SetColor(Ws2812Chain chain, uint8_t led_index, uint8_t red, uint8_t green, uint8_t blue,
                    uint8_t brightness_percent) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    Ws2812Color color = {red, green, blue};
    if (!Ws2812SetPixel(chain, led_index, color, brightness_percent)) {
        return false;
    }

    return Ws2812Show(chain);
}

bool Ws2812Clear(Ws2812Chain chain) {
    ChainState *state = GetChain(chain);
    if (state == NULL) {
        return false;
    }

    Ws2812Color black = {0, 0, 0};
    if (!Ws2812Fill(chain, black, 0)) {
        return false;
    }

    return Ws2812Show(chain);
}
