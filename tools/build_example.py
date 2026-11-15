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

"""Build the standalone example using an installed XC8 compiler and DFP."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xc8', default=os.environ.get('XC8_CC') or shutil.which('xc8-cc'))
    parser.add_argument('--dfp', default=os.environ.get('XC8_DFP'))
    parser.add_argument('--device', choices=('16F1454', '16F1455', '16F1459',
                                                '16LF1454', '16LF1455', '16LF1459'),
                        default='16F1454')
    parser.add_argument('--optimization', choices=('0', '2'), default='0')
    parser.add_argument('--all-pins', action='store_true',
                        help='Link all supported GPIO transmitters for waveform checks')
    args = parser.parse_args()
    if not args.xc8 or not args.dfp:
        parser.error('provide --xc8 and --dfp, or set XC8_CC and XC8_DFP')

    root = Path(__file__).resolve().parents[1]
    output = root / 'build' / (args.device + '-O' + args.optimization)
    if args.all_pins:
        output = output / 'all-pins'
    output.mkdir(parents=True, exist_ok=True)
    compiler = str(Path(args.xc8).resolve())
    dfp = str(Path(args.dfp).resolve())
    command = [compiler, '-mcpu=' + args.device, '-mdfp=' + dfp,
               '-std=c99', '-O' + args.optimization, '-ginhx32',
               '-msummary=-psect,-class,+mem,-hex,-file', '-I' + str(root / 'inc'),
               '-o', str(output / 'example.elf'),
               str(root / 'src/ws2812.c'), str(root / 'src/ws2812_pic16.c'),
               str(root / ('tests/fixtures/waveform_main.c' if args.all_pins else 'examples/main.c'))]
    if args.all_pins:
        command.extend(['-I' + str(root / 'tests/fixtures'),
                        '-DWS2812_CONFIG_HEADER="all_pins.h"'])
    subprocess.run(command, cwd=output, check=True)
    print('Built:', output / 'example.hex')


if __name__ == '__main__':
    main()
