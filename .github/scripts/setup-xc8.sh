#!/usr/bin/env bash
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


set -euo pipefail

if [[ $# -ne 1 || "$1" != /* || "$1" == / ]]; then
    printf 'Usage: %s ABSOLUTE_TOOLCHAIN_DIRECTORY\n' "$0" >&2
    exit 2
fi

toolchain_dir="$1"
compiler_dir="$toolchain_dir/xc8"
dfp_dir="$toolchain_dir/dfp"
download_dir="$toolchain_dir/downloads"
mkdir -p "$download_dir"

download_checked() {
    local url="$1" destination="$2" checksum="$3"
    if [[ ! -f "$destination" ]]; then
        curl --fail --location --retry 3 --connect-timeout 20 --max-time 300 \
            "$url" --output "$destination"
    fi
    printf '%s  %s\n' "$checksum" "$destination" | sha256sum --check --status
}

if [[ ! -x "$compiler_dir/bin/xc8-cc" ]]; then
    installer="$download_dir/xc8-v3.10-linux.run"
    download_checked \
        'https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc8-v3.10-full-install-linux-x64-installer.run' \
        "$installer" \
        '628803b96f468a5981d6bc1d0a5e6c7fa809e4d87e3cca961805e2a857f5846e'
    chmod +x "$installer"

    # Microchip requires root for installation. Keep compiler files in the
    # explicitly supplied directory and avoid changing all users' settings.
    installer_args=(--mode unattended --unattendedmodeui none --netservername localhost
        --LicenseType FreeMode --ModifyAll 0 --prefix "$compiler_dir")
    if [[ $EUID -eq 0 ]]; then
        "$installer" "${installer_args[@]}"
    else
        sudo -- "$installer" "${installer_args[@]}"
        sudo chown -R "$(id -u):$(id -g)" "$compiler_dir"
    fi
fi

if [[ ! -f "$dfp_dir/xc8/pic/include/proc/pic16lf1459.h" ]]; then
    pack="$download_dir/PIC12-16F1xxx_DFP.1.10.270.atpack"
    download_checked \
        'https://packs.download.microchip.com/Microchip.PIC12-16F1xxx_DFP.1.10.270.atpack' \
        "$pack" \
        'cf4f7a040259beea00ff06ea3a4152b62d1a7795a5d6c514a2c9d01736a1fd90'
    python3 -m zipfile -e "$pack" "$dfp_dir"
fi

"$compiler_dir/bin/xc8-cc" --version

if [[ -n "${GITHUB_ENV:-}" ]]; then
    printf 'XC8_CC=%s\nXC8_DFP=%s\n' \
        "$compiler_dir/bin/xc8-cc" "$dfp_dir/xc8" >> "$GITHUB_ENV"
fi
