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

# Exercise the real consumer entrypoint, requiring a specific configuration error.
execute_process(COMMAND "${CMAKE_COMMAND}"
    -S "${SOURCE}/tests/cmake/consumer" -B "${BINARY}"
    -G "${GENERATOR}" "-DCMAKE_C_COMPILER=${COMPILER}"
    "-DCMAKE_C_COMPILER_ARG1=${COMPILER_ARG}" "-DCMAKE_MAKE_PROGRAM=${NINJA}"
    "-DPIC16_RGB_SOURCE_DIR=${SOURCE}" "-DPIC16_RGB_CHAINS=${ROWS}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "${EXPECTED}")
    message(FATAL_ERROR "Expected configuration rejection ${EXPECTED}: ${output}${error}")
endif()
