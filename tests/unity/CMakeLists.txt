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

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/Unity.cmake")

foreach(capacity IN ITEMS 3 20)
    foreach(suite IN ITEMS ws2812 uninitialized)
        set(test_target test_${suite}_${capacity})
        add_executable(${test_target}
            test_${suite}.c
            fakes/ws2812_port_fake.c
            "${PROJECT_SOURCE_DIR}/src/ws2812.c")
        target_include_directories(${test_target} PRIVATE
            "${PROJECT_SOURCE_DIR}/inc"
            "${PROJECT_SOURCE_DIR}/src"
            fakes)
        target_compile_definitions(${test_target} PRIVATE WS2812_MAX_LEDS=${capacity})
        set_target_properties(${test_target} PROPERTIES
            C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
        target_link_libraries(${test_target} PRIVATE pic16_rgb_unity)
        if(MSVC)
            target_compile_options(${test_target} PRIVATE /W4 /WX)
        else()
            target_compile_options(${test_target} PRIVATE -Wall -Wextra -Werror)
        endif()
        add_test(NAME ${test_target} COMMAND ${test_target})
    endforeach()
endforeach()

foreach(device IN ITEMS 16F1454 16F1455 16F1459 16LF1454 16LF1455 16LF1459)
    set(port_test test_pic16_port_${device})
    add_executable(${port_test} test_pic16_port.c
        "${PROJECT_SOURCE_DIR}/src/ws2812_pic16.c")
    target_include_directories(${port_test} PRIVATE
        "${PROJECT_SOURCE_DIR}/inc" "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures" fakes)
    target_compile_definitions(${port_test} PRIVATE
        _${device}=1 WS2812_CONFIG_HEADER="all_pins.h")
    set_target_properties(${port_test} PROPERTIES
        C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    target_link_libraries(${port_test} PRIVATE pic16_rgb_unity)
    if(MSVC)
        target_compile_options(${port_test} PRIVATE /W4 /WX)
    else()
        target_compile_options(${port_test} PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME ${port_test} COMMAND ${port_test})
endforeach()

add_executable(test_chains test_chains.c fakes/ws2812_port_fake.c
    "${PROJECT_SOURCE_DIR}/src/ws2812.c")
target_include_directories(test_chains PRIVATE "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures" fakes)
target_compile_definitions(test_chains PRIVATE WS2812_CONFIG_HEADER="multi_chains.h")
target_link_libraries(test_chains PRIVATE pic16_rgb_unity)
set_target_properties(test_chains PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
add_test(NAME test_chains COMMAND test_chains)
