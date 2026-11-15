# Copyright 2026 Maxim Pavlov
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(PIC16RGB VERSION 0.1.0 LANGUAGES C)

# Compile the source in the consuming firmware target, inheriting its exact
# device/DFP/optimization flags, as with MLA's source-list integration.
add_library(pic16_rgb INTERFACE)
add_library(PIC16RGB::ws2812 ALIAS pic16_rgb)
target_sources(pic16_rgb INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812_pic16.c")
target_include_directories(pic16_rgb INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc")

set(PIC16_RGB_MAX_LEDS 20 CACHE STRING "Maximum buffered chain length (1-20)")
if(NOT PIC16_RGB_MAX_LEDS MATCHES "^([1-9]|1[0-9]|20)$")
    message(FATAL_ERROR "PIC16_RGB_MAX_LEDS must be an integer from 1 to 20")
endif()
target_compile_definitions(pic16_rgb INTERFACE
    WS2812_MAX_LEDS=${PIC16_RGB_MAX_LEDS})

# One row per physical output. The generated header is also usable by projects
# that compile the sources directly instead of linking this interface target.
set(PIC16_RGB_CHAINS "Main:RC5:${PIC16_RGB_MAX_LEDS}" CACHE STRING
    "Semicolon-separated Name:Pin:Capacity chain declarations")
set(chain_rows "")
set(chain_names "")
set(chain_pins "")
foreach(row IN LISTS PIC16_RGB_CHAINS)
    if(NOT row MATCHES "^([A-Za-z][A-Za-z0-9_]*):(RA[45]|RC[0-5]):([1-9]|1[0-9]|20)$")
        message(FATAL_ERROR "Invalid PIC16_RGB_CHAINS row: ${row}")
    endif()
    set(chain_name "${CMAKE_MATCH_1}")
    set(chain_pin "${CMAKE_MATCH_2}")
    set(chain_capacity "${CMAKE_MATCH_3}")
    if(chain_name IN_LIST chain_names OR chain_pin IN_LIST chain_pins)
        message(FATAL_ERROR "Duplicate chain name or GPIO in: ${row}")
    endif()
    list(APPEND chain_names "${chain_name}")
    list(APPEND chain_pins "${chain_pin}")
    string(APPEND chain_rows " X(${chain_name}, ${chain_pin}, ${chain_capacity})")
endforeach()
if(NOT chain_rows)
    message(FATAL_ERROR "PIC16_RGB_CHAINS must declare at least one chain")
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/ws2812_chains.h"
    CONTENT "// Generated from PIC16_RGB_CHAINS; do not edit.\n#define WS2812_CHAINS(X)${chain_rows}\n")
target_include_directories(pic16_rgb INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(pic16_rgb INTERFACE WS2812_CONFIG_HEADER="ws2812_chains.h")

# Firmware consumers neither build nor download any test dependencies.
option(PIC16_RGB_BUILD_TESTS "Build native Unity API tests" OFF)
if(PIC16_RGB_BUILD_TESTS)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Unity tests need a separate native host build")
    endif()
    enable_testing()
    add_subdirectory(tests)
endif()
