# Contributing

Thank you for your interest in contributing to PIC16-RGB! Bug reports, driver
improvements, tests, and documentation updates are welcome.

Keep each contribution focused on one change so it is easy to review. Explain
the problem you are solving and the resulting behavior in your pull request.

- Create a `feature/{heading}` branch with a short, descriptive heading, such as
  `feature/add-patterns`.
- Follow the code style and documentation conventions in [AGENTS.md](AGENTS.md).
- Add or update relevant tests and keep the README and public API contracts
  accurate when behavior changes.
- Submit changes through a pull request to the default branch. Include a brief
  description and your test results; all CI checks must pass before merging.

Every commit must include both a sign-off and a cryptographic signature:

- `-s` adds a `Signed-off-by` trailer using your Git name and email.
- `-S` signs the commit with your configured GPG or SSH signing key.

```sh
git switch -c feature/add-patterns
git commit -s -S -m "Describe the change"
```

Keep Apache 2.0 license headers on new source files and preserve third-party
license notices. See [LICENSE](LICENSE) for the project license.

## Build and Test

Run these commands from the repository root. For code changes, run the native
tests. For changes affecting target code or timing, also build and check the
waveform for every affected MCU at both supported optimization levels. Shared
transmitter changes require the full six-device matrix below.

### Native Tests

Use CMake 3.20 or newer and a host C compiler, such as GCC, Clang, or MSVC.
For MSVC, run from a Visual Studio developer shell.

```sh
cmake -S . -B build/native -DPIC16_RGB_BUILD_TESTS=ON
cmake --build build/native --config Debug
ctest --test-dir build/native -C Debug --output-on-failure
```

The first configure downloads the pinned Unity dependency. For offline use, add
`-DFETCHCONTENT_SOURCE_DIR_UNITY=/path/to/unity` with an existing checkout of that
version. Keep this host build separate from XC8 builds. These tests cover the
public API, independent chains, GPIO setup, and build configuration; see
[tests/README.md](tests/README.md) for details.

### XC8 Setup

Install Python 3.10 or newer, MPLAB XC8, and the PIC12-16F1xxx device family pack.
The tested toolchain is XC8 3.10 with DFP 1.10.270. Set the following paths for
your installation, or pass them as `--xc8` and `--dfp` to the build helper.
The DFP path must point to the pack's `xc8` subdirectory.

PowerShell:

```powershell
$env:XC8_CC = 'C:/Program Files/Microchip/xc8/v3.10/bin/xc8-cc.exe'
$env:XC8_DFP = "$env:USERPROFILE/.mchp_packs/Microchip/PIC12-16F1xxx_DFP/1.10.270/xc8"
```

Bash (replace the example paths):

```bash
export XC8_CC=/path/to/xc8/v3.10/bin/xc8-cc
export XC8_DFP=/path/to/PIC12-16F1xxx_DFP/1.10.270/xc8
```

### Build One MCU

Select the exact device with `--device`:

| MCU | Argument |
|---|---|
| PIC16F1454 | `16F1454` |
| PIC16F1455 | `16F1455` |
| PIC16F1459 | `16F1459` |
| PIC16LF1454 | `16LF1454` |
| PIC16LF1455 | `16LF1455` |
| PIC16LF1459 | `16LF1459` |

For example, build the three-pixel application for PIC16LF1455 at `-O2` and
check its emitted waveform:

```sh
python tools/build_example.py --device 16LF1455 --optimization 2
python tests/python/test_waveform.py build/16LF1455-O2/example.hex
```

Then build and check the fixture that includes every supported GPIO transmitter:

```sh
python tools/build_example.py --device 16LF1455 --optimization 2 --all-pins
python tests/python/test_waveform.py build/16LF1455-O2/all-pins/example.hex
```

Repeat with `--optimization 0` and the corresponding `16LF1455-O0` output paths.
The ordinary example includes clock startup and can be used on suitable hardware.
The `--all-pins` fixture is for instruction analysis and must not be flashed.
Check compiler messages for optimization substitutions caused by licensing;
report the mode actually used.

### Check All MCUs

These loops build both the application and the all-pin fixture for all six
devices at `-O0` and `-O2`, checking each HEX file immediately after building it.
They stop on the first failure.

PowerShell:

```powershell
$devices = '16F1454', '16F1455', '16F1459', '16LF1454', '16LF1455', '16LF1459'
foreach ($device in $devices) {
    foreach ($optimization in 0, 2) {
        foreach ($allPins in $false, $true) {
            $buildArgs = @('--device', $device, '--optimization', $optimization)
            $outputDir = "build/$device-O$optimization"
            if ($allPins) {
                $buildArgs += '--all-pins'
                $outputDir += '/all-pins'
            }

            python tools/build_example.py @buildArgs
            if ($LASTEXITCODE -ne 0) { throw "Build failed: $outputDir" }
            python tests/python/test_waveform.py "$outputDir/example.hex"
            if ($LASTEXITCODE -ne 0) { throw "Waveform check failed: $outputDir" }
        }
    }
}
```

Bash (use `python3` instead of `python` if required by your installation):

```bash
(
    set -e
    for device in 16F1454 16F1455 16F1459 16LF1454 16LF1455 16LF1459; do
        for optimization in 0 2; do
            for variant in example all-pins; do
                args=(--device "$device" --optimization "$optimization")
                output_dir="build/${device}-O${optimization}"
                if [ "$variant" = all-pins ]; then
                    args+=(--all-pins)
                    output_dir+=/all-pins
                fi

                python tools/build_example.py "${args[@]}"
                python tests/python/test_waveform.py "$output_dir/example.hex"
            done
        done
    done
)
```

### Report Results

Before submitting, run `git diff --check`. Include native test results, the XC8
and DFP versions, devices, optimization levels, and waveform-check results in
your pull request. CI currently checks XC8 builds for PIC16F1454 at both levels;
changes affecting other targets also need the relevant local checks.

Report hardware tests separately, including the MCU, LED type, GPIO, chain
length, and measured waveform if available. Compilation and instruction checks
do not establish electrical timing or correct LED operation on hardware. For
documentation-only changes, check links and command examples; firmware builds
are not required.
