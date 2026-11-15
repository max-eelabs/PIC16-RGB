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

foreach(scenario IN ITEMS duplicate_pin duplicate_name unsupported zero too_many empty)
    if(scenario STREQUAL "duplicate_pin")
        set(rows "Main:RC5:3;Aux:RC5:2")
        set(expected "Duplicate chain")
    elseif(scenario STREQUAL "duplicate_name")
        set(rows "Main:RC5:3;Main:RC2:2")
        set(expected "Duplicate chain")
    elseif(scenario STREQUAL "unsupported")
        set(rows "Main:RA3:3")
        set(expected "Invalid PIC16_RGB_CHAINS")
    elseif(scenario STREQUAL "zero")
        set(rows "Main:RC5:0")
        set(expected "Invalid PIC16_RGB_CHAINS")
    elseif(scenario STREQUAL "too_many")
        set(rows "Main:RC5:21")
        set(expected "Invalid PIC16_RGB_CHAINS")
    else()
        set(rows "")
        set(expected "must declare at least one chain")
    endif()
    add_test(NAME configuration_${scenario}
        COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${PROJECT_SOURCE_DIR}"
            "-DBINARY=${CMAKE_CURRENT_BINARY_DIR}/config_${scenario}"
            "-DGENERATOR=${CMAKE_GENERATOR}" "-DCOMPILER=${CMAKE_C_COMPILER}"
            "-DCOMPILER_ARG=${CMAKE_C_COMPILER_ARG1}" "-DNINJA=${CMAKE_MAKE_PROGRAM}"
            "-DROWS=${rows}" "-DEXPECTED=${expected}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check_configuration.cmake")
endforeach()

add_test(NAME cmake_consumer
    COMMAND "${CMAKE_COMMAND}"
        -S "${CMAKE_CURRENT_SOURCE_DIR}/consumer"
        -B "${CMAKE_CURRENT_BINARY_DIR}/consumer"
        -G "${CMAKE_GENERATOR}"
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_C_COMPILER_ARG1=${CMAKE_C_COMPILER_ARG1}"
        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DPIC16_RGB_SOURCE_DIR=${PROJECT_SOURCE_DIR}")

add_test(NAME cmake_consumer_build COMMAND "${CMAKE_COMMAND}" --build
    "${CMAKE_CURRENT_BINARY_DIR}/consumer")
set_tests_properties(cmake_consumer_build PROPERTIES DEPENDS cmake_consumer)
