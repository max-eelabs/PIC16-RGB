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
project(PIC16RGBConsumerCheck LANGUAGES C)

if(NOT DEFINED PIC16_RGB_SOURCE_DIR)
    message(FATAL_ERROR "Pass the directory of the dependency under test")
endif()

# A consumer chooses capacity before bringing the dependency into its build.
set(PIC16_RGB_MAX_LEDS 7 CACHE STRING "Consumer capacity")
add_subdirectory("${PIC16_RGB_SOURCE_DIR}" dependency)
add_library(consumer INTERFACE)
target_link_libraries(consumer INTERFACE PIC16RGB::ws2812)

if(PIC16_RGB_BUILD_TESTS OR TARGET pic16_rgb_unity)
    message(FATAL_ERROR "Firmware dependency unexpectedly enabled its tests")
endif()

get_target_property(sources PIC16RGB::ws2812 INTERFACE_SOURCES)
list(LENGTH sources source_count)
if(NOT source_count EQUAL 2)
    message(FATAL_ERROR "Expected the core and PIC backend sources")
endif()
foreach(relative IN ITEMS src/ws2812.c src/ws2812_pic16.c)
    if(NOT "${PIC16_RGB_SOURCE_DIR}/${relative}" IN_LIST sources)
        message(FATAL_ERROR "Missing consumer source: ${relative}")
    endif()
endforeach()

get_target_property(includes PIC16RGB::ws2812 INTERFACE_INCLUDE_DIRECTORIES)
if(NOT "${PIC16_RGB_SOURCE_DIR}/inc" IN_LIST includes OR
   "${PIC16_RGB_SOURCE_DIR}/src" IN_LIST includes)
    message(FATAL_ERROR "Expected public headers and generated configuration, no private src headers")
endif()
get_target_property(definitions PIC16RGB::ws2812 INTERFACE_COMPILE_DEFINITIONS)
if(NOT "WS2812_MAX_LEDS=7" IN_LIST definitions)
    message(FATAL_ERROR "Consumer capacity did not reach the public target")
endif()

# Compile the exported public interface with a host compiler; the PIC assembly
# itself is built with XC8 in the separate target-validation matrix.
add_library(header_check OBJECT configuration_probe.c)
target_include_directories(header_check PRIVATE ${includes})
target_compile_definitions(header_check PRIVATE ${definitions})
