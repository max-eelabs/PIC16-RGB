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

"""Check XC8-emitted PIC instructions, including every data value at byte boundaries.

This is a focused transmitter interpreter, not a full MCU/electrical simulator.
It rejects unexpected instructions and checks all high/low intervals between bits.
"""

import argparse
from pathlib import Path


def check_waveform(hex_path):
    memory = {}
    base = 0
    for line in hex_path.read_text().splitlines():
        record = bytes.fromhex(line.removeprefix(":"))
        assert sum(record) % 256 == 0, "invalid HEX checksum"
        size, kind = record[0], record[3]
        if kind == 4:
            base = int.from_bytes(record[4:6], "big") << 16
        elif kind == 0:
            address = base + int.from_bytes(record[1:3], "big")
            memory.update((address + i, value)
                          for i, value in enumerate(record[4:4 + size]))

    def word(address):
        return memory.get(address, 255) | (memory.get(address + 1, 255) << 8)

    starts = [address + 2 for address in memory if address % 2 == 0
              and word(address) == 0x0022 and (word(address + 2) & 0x3c7f) in (0x140c, 0x140e)]
    assert starts, "expected at least one GPIO transmitter in LAT bank 2"

    # Exercise every byte value in every position of one- and three-pixel
    # frames, plus maximum-length frames and the final loop iteration.
    samples = []
    for count in (1, 3, 20):
        size = 3 * count
        samples.extend((bytes(size), bytes([255] * size),
                        bytes([0x55, 0xaa, 0x81] * count)))
        if count <= 3:
            for position in range(size):
                for value in range(256):
                    data = bytearray([0x55, 0xaa, 0x81] * count)
                    data[position] = value
                    samples.append(bytes(data))
    for transmitter in starts:
        for data in samples:
            cursor = transmitter
            cycles = byte_index = 0
            remaining = len(data)
            level = False
            edges = []
            for _ in range(200 * len(data)):
                instruction = word(cursor)
                cursor += 2
                if instruction == 0:
                    cycles += 1
                elif (instruction & 0x3c7f) in (0x140c, 0x100c, 0x140e, 0x100e):
                    assert (instruction & 0x03ff) == (word(transmitter) & 0x03ff), "wrong GPIO touched"
                    cycles += 1
                    new_level = bool(instruction & 0x0400)
                    if level != new_level:
                        edges.append((cycles, new_level))
                        level = new_level
                elif instruction & 0x3c7f in (0x1800, 0x1c00):
                    bit = (instruction >> 7) & 7
                    is_set = bool(data[byte_index] & (1 << bit))
                    skip = is_set if instruction & 0x0400 else not is_set
                    cycles += 2 if skip else 1
                    cursor += 2 if skip else 0
                elif instruction == 0x3101:  # ADDFSR 0,1 between GRB bytes.
                    cycles += 1
                    byte_index += 1
                elif instruction == 0x0b89:  # DECFSZ WREG,F: remaining bytes.
                    remaining -= 1
                    cycles += 2 if remaining == 0 else 1
                    cursor += 2 if remaining == 0 else 0
                elif instruction & 0x3e00 == 0x3200:  # Signed relative BRA.
                    offset = instruction & 0x1ff
                    if offset & 0x100:
                        offset -= 0x200
                    cursor += 2 * offset
                    cycles += 2
                else:
                    break
            else:
                raise AssertionError("waveform did not terminate")

            assert len(edges) == 16 * len(data) and not level
            assert byte_index == len(data) and remaining == 0
            decoded = []
            for index in range(8 * len(data)):
                rising = edges[2 * index][0]
                falling = edges[2 * index + 1][0]
                high_ns = (falling - rising) * 1000 / 12
                is_one = high_ns >= 580
                assert (580 <= high_ns <= 1000) if is_one else (220 <= high_ns <= 380)
                decoded.append(int(is_one))
                if index < 8 * len(data) - 1:
                    next_rising = edges[2 * index + 2][0]
                    low_ns = (next_rising - falling) * 1000 / 12
                    assert 580 <= low_ns <= 1000
                    assert high_ns + low_ns >= 1250
            actual = bytes(sum(decoded[start + i] << (7 - i) for i in range(8))
                           for start in range(0, 8 * len(data), 8))
            assert actual == data, (actual, data)
    print(f"PASS: emitted PIC waveform decodes {len(samples)} GRB frames on each of {len(starts)} GPIO transmitters within V6 timing")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("hex_file", type=Path)
    args = parser.parse_args()
    check_waveform(args.hex_file)
