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

# Native tests only. Keep third-party sources in CMake's build-tree _deps.
include_guard(GLOBAL)
include(FetchContent)

# Unity v2.7.0, pinned to an immutable commit and verified archive checksum.
# Populate src/ without adding upstream's CMake project: this project owns the
# small test target and its compiler options, as with MLA source integration.
FetchContent_Declare(unity
    URL "https://codeload.github.com/ThrowTheSwitch/Unity/zip/b6763fbd9cedfacaa89e2ad9fd00d615a234e355"
    URL_HASH SHA256=e57eb2a1b3ed07df07e73b250e2f39aef3c4edb079440dae5614c1b8a31ff54b
    TIMEOUT 60
    INACTIVITY_TIMEOUT 20
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    SOURCE_SUBDIR src)

FetchContent_MakeAvailable(unity)

add_library(pic16_rgb_unity OBJECT "${unity_SOURCE_DIR}/src/unity.c")
target_include_directories(pic16_rgb_unity PUBLIC "${unity_SOURCE_DIR}/src")
target_compile_definitions(pic16_rgb_unity PUBLIC
    UNITY_EXCLUDE_FLOAT UNITY_EXCLUDE_DOUBLE)
