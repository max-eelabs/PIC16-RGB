# PIC16-RGB

A standalone C driver for independent chains of **WS2812B-V6** RGB LEDs on
**PIC16(L)F1454/5/9**, compiled with **MPLAB XC8**, at a **48 MHz CPU
clock**. Each chain uses its own GPIO and supports indexed LEDs. The default
configuration is one RC5 chain; RA4, RA5 and RC0-RC5 are supported outputs.

Supported MCU targets are **PIC16F1454, PIC16F1455, PIC16F1459,
PIC16LF1454, PIC16LF1455, and PIC16LF1459**. Both F and LF versions use the
same 48 MHz instruction-timed transmitter on the supported GPIOs. Select the
exact device with XC8's `-mcpu` option (or `--device` in the example build tool).
Package and temperature-grade suffixes do not select a different driver backend;
use the package pinout and electrical limits for the actual ordering code.

The driver offers indexed colors, per-assignment brightness, buffered batch
updates, immediate updates, fill/clear, effective-color readback, and dirty-frame
tracking, independently for each chain. It uses static storage and a bounded assembly byte loop. No heap,
interrupt handler, timer, DMA, SPI, or PWM peripheral generates the LED waveform.

## Layout and consumption

The `src/` and `inc/` split follows the [Microchip MLA USB repository](https://github.com/MicrochipTech/mla_usb/tree/d28cc13c6b4a539ed2333348efbde7c6546fbfc6).
Public headers belong in `inc/`; implementation and private headers belong in
`src/`. Tests and examples remain outside the library source directories.

```text
CMakeLists.txt
inc/
  ws2812.h                   Public explicit-chain API
  ws2812_config.h            Shared build-time chain declarations
src/
  ws2812.c                   Color buffer, scaling, public operations
  ws2812_port.h              Private synchronous hardware contract
  ws2812_pic16.c             Selected GPIO setup, interrupt handling, timed assembly
examples/
  main.c                     Clock/configuration and three-pixel example
tests/
  CMakeLists.txt            Native test entry point
  README.md                 Test groups and execution instructions
  unity/                    Unity C suites and their CMake target definitions
    fakes/                  Link-time frame capture and fake XC8 registers
  python/
    test_waveform.py        Interpreter of real XC8-emitted HEX instructions
  cmake/                    Configuration checks and their CTest definitions
    consumer/               Minimal downstream CMake project and header probe
  fixtures/                 Shared chain configurations and XC8 waveform entry point
cmake/
  Unity.cmake                Pinned FetchContent dependency for native tests
tools/
  build_example.py           Standalone XC8 build helper
AGENTS.md
```

The firmware needs both C files and `inc/`; it does not depend on Unity or Python.
The core is compiled unchanged for native tests, linked against the fake port
instead of `ws2812_pic16.c`. Hardware-specific behavior stays in the real port
and is checked separately with XC8 builds, instruction analysis, and hardware.

### Existing MLA-style source integration

Use this inside the consuming firmware after FetchContent has populated the
repository. `pic16_rgb_SOURCE_DIR` below is CMake's populated source path:

```cmake
target_sources(my_firmware PRIVATE
    "${pic16_rgb_SOURCE_DIR}/src/ws2812.c"
    "${pic16_rgb_SOURCE_DIR}/src/ws2812_pic16.c")
target_include_directories(my_firmware PRIVATE
    "${pic16_rgb_SOURCE_DIR}/inc")
target_compile_definitions(my_firmware PRIVATE WS2812_MAX_LEDS=20)
```

This matches MLA's manual source-list approach and works with generated MPLAB
compile targets. Do not also link the optional target below, or the driver would
be compiled twice. Keep the application's `user.cmake` as the customization point.

### Optional CMake target

With `FetchContent_MakeAvailable(pic16_rgb)` or a local `add_subdirectory`, link:

```cmake
set(PIC16_RGB_MAX_LEDS 20 CACHE STRING "Maximum buffered LED count")
add_subdirectory(path/to/PIC16-RGB)
target_link_libraries(my_firmware PRIVATE PIC16RGB::ws2812)
```

The namespaced target carries the source files, public include directory, and
capacity and generated chain definitions. It is a CMake INTERFACE source target: the consumer compiles
both C files with its exact XC8 device/DFP/optimization settings. It is not a
precompiled archive and does not globally change compiler options. In particular,
this avoids an independently compiled library missing MPLAB's per-target flags.
Private `src/` headers are not exported as an include directory.

Host tests default to OFF, including when fetched as a dependency. Enabling them
requires a separate native build, not the firmware's XC8 cross build. Native
tests fetch Unity v2.7.0 at immutable commit
`b6763fbd9cedfacaa89e2ad9fd00d615a234e355` through `cmake/Unity.cmake`, using a
SHA256-verified archive, like the firmware's MLA USB dependency. Unity sources
and their MIT license stay under the native build's `_deps/unity-src/`; no Unity
code is stored in this repository. Firmware consumers neither fetch nor compile
Unity.

The source repository is [max-eelabs/PIC16-RGB](https://github.com/max-eelabs/PIC16-RGB).
Declare a pinned commit/archive with FetchContent just as for MLA, then choose
either integration approach above. Local add_subdirectory integration also works.

## Multiple-chain contract and configuration

Choose names, pins and capacities at build time. Every translation unit must use
the same table. For CMake consumers, configure before adding the dependency:

```cmake
set(PIC16_RGB_CHAINS "Main:RC5:3;Aux:RC2:5" CACHE STRING "LED chains")
add_subdirectory(path/to/PIC16-RGB)
target_link_libraries(my_firmware PRIVATE PIC16RGB::ws2812)
```

This generates `ws2812_chains.h` and exports its include directory and
`WS2812_CONFIG_HEADER` definition through the interface target. To change an
existing build, pass `-DPIC16_RGB_CHAINS="Main:RC5:3;Aux:RC2:5"` when configuring.
The legacy `PIC16_RGB_MAX_LEDS` setting supplies only the initial fallback table's
capacity; explicit or cached `PIC16_RGB_CHAINS` declarations take precedence.

For direct source inclusion, create an application header such as `led_chains.h`:

```c
#define WS2812_CHAINS(X) \
    X(Main, RC5, 3) \
    X(Aux, RC2, 5)
```

Add its directory to the include search path and define
`WS2812_CONFIG_HEADER="led_chains.h"` consistently for all source files. Chain
names produce identifiers such as `kWs2812ChainMain` and `kWs2812ChainAux`.
Duplicate pins/names, unsupported pins, empty tables, and invalid capacities
are rejected. Direct source users can also use the generated CMake header.

```c
Ws2812Init(kWs2812ChainMain, 3);  // RC5 -> three daisy-chained LEDs.
Ws2812Init(kWs2812ChainAux, 5);   // RC2 -> five daisy-chained LEDs.
const Ws2812Color red = {255, 0, 0};
const Ws2812Color blue = {0, 0, 255};
Ws2812SetPixel(kWs2812ChainMain, 2, red, 30);
Ws2812SetPixel(kWs2812ChainAux, 4, blue, 50);
Ws2812Show(kWs2812ChainMain);     // Does not transmit Aux or clear its dirty flag.
Ws2812ShowIfDirty(kWs2812ChainAux);
```

Each chain has independent active length, colors and dirty state. Initialization,
clear, fill, readback and show affect only that chain. Invalid identifiers return
false (count returns zero) without changing any chain or hardware. Initialization
on one chain leaves other chains uninitialized until explicitly initialized.
Updating one pixel still transmits the full active length of its own chain.
Transmission is blocking, sequential, and non-reentrant across all chains.
No caller buffers are retained; no heap allocation is used.

This is a source-breaking update to the unpublished API: add the chain identifier
as the first argument to every operation. No implicit default-chain wrappers exist.

## Wiring

```text
PIC RC5 ----> LED0 DIN
              LED0 DOUT ----> LED1 DIN
                              LED1 DOUT ----> LED2 DIN
                                              LED2 DOUT unused

LED supply + ----+---------------+---------------+
                LED0 VDD        LED1 VDD        LED2 VDD
Common ground --+---------------+---------------+---- PIC VSS
                LED0 VSS        LED1 VSS        LED2 VSS
```

Power is distributed in parallel; data passes through the pixels sequentially.
The PIC pin carries data, not LED operating current. Choose the supply and board
layout for the actual LED load and logic levels. Check package orientation:
For PIC16(L)F1454/1455, RC5 is pin 5 on 14-pin packages and pin 4 on the
16-pin QFN/UQFN packages. For PIC16(L)F1459, RC5 is pin 5 on 20-pin
PDIP/SOIC/SSOP packages and pin 2 on 20-pin QFN/UQFN packages. Configure
logical GPIO names, not package pin numbers. See the family datasheet's
[pin allocation tables 1-3](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/PIC16LF1454-5-9-14-20-Pin-8-Bit-Flash-USB-Microcontroller-with-XLP-Technology-DS40001639C.pdf#page=5).

F and LF variants require different supply wiring and ratings. LF devices have
a 3.6 V maximum operating supply; their VUSB3V3 pin connects to VDD at the PCB.
At the driver's required 48 MHz, select VDD using the frequency/voltage limits
in the datasheet, not the low-frequency minimum supply rating. A low-voltage
GPIO must also meet the powered LED's DIN threshold; use a suitable level
shifter when necessary. See the [family electrical specifications and pin
descriptions](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/PIC16LF1454-5-9-14-20-Pin-8-Bit-Flash-USB-Microcontroller-with-XLP-Technology-DS40001639C.pdf).
LED package pin numbers are a separate pinout.

## Quick start

Establish a stable 48 MHz CPU clock and release peripheral ownership of each
selected GPIO. Initialization disables PWM1 for RC5, or PWM2 for RC3 on
1454/1455 only, and clears only the selected pin's analog bit where applicable.
It leaves clock, supply,
watchdog, USB, and Timer2 configuration to the application. A complete bare-metal
startup is in `examples/main.c`.

```c
#include "ws2812.h"

// Once after clock/GPIO startup; this immediately latches three black pixels.
bool ready = Ws2812Init(kWs2812ChainMain, 3);

// When ready is true, stage several colors and transmit a single complete frame.
const Ws2812Color colors[3] = {
    {255, 0, 0},   // Pixel 0: red.
    {0, 255, 0},   // Pixel 1: green.
    {0, 0, 255}    // Pixel 2: blue.
};
if (ready && Ws2812SetPixels(kWs2812ChainMain, 0, colors, 3, 30)) {
    Ws2812Show(kWs2812ChainMain);
}

// Or change one pixel and immediately transmit the complete stored chain.
Ws2812SetColor(kWs2812ChainMain, 1, 0, 255, 0, 30);

// Before sleep, synchronously send black to every active pixel.
Ws2812Clear(kWs2812ChainMain);
```

Indexes are zero-based: index 0 is nearest the PIC. Updating one pixel preserves
the stored values of all other active pixels. A one-wire frame still transmits
every active pixel; there is no random-access packet addressed to one LED.

## API

Every function takes a `Ws2812Chain` identifier and is main-context only.
The driver is not reentrant, including across different chains; ISR calls
or concurrent edits during a transmission are unsupported. Header comments are
the authoritative contracts, including null pointers and zero-length ranges.

| Function | Behavior |
|---|---|
| `Ws2812Init(chain, count)` | Set active length, reset output, immediately clear chain |
| `Ws2812Count(chain)` | Active count, or zero before initialization |
| `Ws2812SetPixel(chain, index, color, percent)` | Stage one scaled color |
| `Ws2812SetPixels(chain, first, colors, count, percent)` | Validate whole range, then copy/scale it |
| `Ws2812Fill(chain, color, percent)` | Stage one color across all active LEDs |
| `Ws2812GetPixel(chain, index, &color)` | Read effective scaled RGB from the buffer |
| `Ws2812IsDirty(chain)` | Whether staged colors changed since the last Show |
| `Ws2812Show(chain)` | Unconditionally send the complete buffer and latch |
| `Ws2812ShowIfDirty(chain)` | Send only when dirty; clean no-op still returns true |
| `Ws2812SetColor(chain, index, r, g, b, percent)` | Stage one pixel, then unconditional Show |
| `Ws2812Clear(chain)` | Discard pending edits, stage all black, unconditional Show |

Boolean operation results indicate valid local execution, not an acknowledgement
from the LEDs. Every operation affects only the selected chain. Invalid calls leave colors, dirty state, and hardware unchanged.
`GetPixel()` leaves its output unchanged on error. Before initialization, queries
report zero/clean and other operations return false. A zero-length `SetPixels`
is allowed only after initialization with `first <= active count`; its array may
be null. No caller pointer is retained after any call.

Brightness is linear, per assignment, and clamped to 100:

```text
effective = (channel * min(percent, 100) + 50) / 100
```

For example, 255 at 30% becomes 77. There is no gamma correction or retained
unscaled color. `GetPixel()` therefore returns 77, not 255, for that channel.
Dirty tracking compares effective values and is conservative: changing a value
and changing it back still leaves dirty set until Show. Use unconditional Show
after LED power loss because software cannot detect that physical state changed.

Reinitialization clears the new active length. If shortening a physical chain,
clear the old length first so trailing LEDs do not retain their previous colors.

## How the wire protocol works

Each pixel takes one 24-bit value, in **green, red, blue** order, with each byte
sent most-significant bit first. It consumes its own value and forwards subsequent
values to the next pixel. Three LEDs require nine bytes in one frame. A final
low interval latches the colors. Calling a one-pixel transmitter three times
with a latch after every call does not create a three-pixel frame.

The implementation stores scaled values directly in GRB wire order, removing
color conversion and multiplication from the timed transmission. Buffered edits
can happen before Show, with interrupts enabled according to the caller's state.
Show sends one uninterrupted frame and then a 300 us low latch interval.

The target is specifically the V6 timing document. Its page 4 defines zero-high
as 220-380 ns, one-high and either data-low as 580-1000 ns, bit periods of at
least 1.25 us, and reset low greater than 280 us. Other LED revisions/protocols
need their own timing checks. [Worldsemi WS2812B-V6 datasheet, pages 1-4](https://datasheet.lcsc.com/datasheet/pdf/0689d8fd6dabfc7959e82552d4ffad8b.pdf?productCode=C52917433)

## Assembly transmitter

The enhanced mid-range core uses four oscillator periods per instruction cycle.
At 48 MHz that is 12 MHz instruction timing, or about 83.333 ns per cycle. The
source's `WS_BIT` macro is deliberately assembly rather than C so the compiler
cannot insert variable work between edges.

Each configured chain gets its own literal-pin transmitter. Dispatch occurs
before GIE masking. The assembly begins with `banksel LATA` or `banksel LATC`. FSR0 points to the first GRB byte, and
WREG contains `3 * active_count`. INDF0 exposes the current byte regardless of
the selected bank; WREG is also accessible in every bank. This lets the loop keep
BSR on the selected latch bank while testing data and counting remaining bytes.

Each bit does the following:

1. Set the configured latch bit and wait through fixed padding.
2. Use `BTFSS INDF0,bit` to clear the output early for zero.
3. Clear the output later for one.
4. Pad the low interval; `BTFSC` plus `BRA $+1` adds one extra cycle for a one.

| Bit value | High cycles | Low cycles to next bit | High | Low | Bit period |
|---|---:|---:|---:|---:|---:|
| 0 | 4 | 11 | 333 ns | 917 ns | 1.250 us |
| 1 | 8 | 8 | 667 ns | 667 ns | 1.333 us |

These nominal intervals include byte/pixel boundaries when the next bit exists.
The final data bit is followed by the frame's reset low, not another ordinary
data-low interval. Timing assumes the actual oscillator meets the required
accuracy; `_XTAL_FREQ` is a compile-time declaration, not clock configuration.

Bits 7 through 1 use four low-padding NOPs. For bit 0, the final four padding
cycles are replaced with byte-advance and loop control, after testing the old
byte's final bit:

```asm
; Final bit's data-dependent low padding precedes this sequence.
addfsr 0,1          ; 1 cycle: advance to next byte.
decfsz WREG,f      ; 1 cycle when another byte remains.
bra ws2812_byte_Main ; 2 cycles: next byte, total loop overhead 4 cycles.
```

On the final byte, DECFSZ takes its skip path and omits the branch. DIN is already
low, so falling out into latch handling does not create another data edge. The
byte loop means program size does not grow by duplicating bit assembly per LED.
Do not insert ordinary C between bytes or add loop instructions without replacing
padding and recounting both zero/one paths.

FSR0 and WREG are used during transmission; callers must not depend on their
values across the C API. The previous GIE state is saved and restored. If GIE was
clear on entry, it stays clear through the latch; the library never forces it on.
An ISR allowed during the latch can extend its duration, but must not toggle any driver-owned GPIO,
change the CPU clock, reenter this driver, or corrupt its stored colors.

Core, memory, and pin definitions are documented in the
[PIC16(L)F1454/5/9 datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/PIC16LF1454-5-9-14-20-Pin-8-Bit-Flash-USB-Microcontroller-with-XLP-Technology-DS40001639C.pdf),
sections 2, 3, 12, and 28. Assembly-compatible instructions alone do not establish
register, pin, clock, or compiler compatibility on another PIC16.

## Capacity, latency, and integration limits

Each chain capacity is independently configured from 1 to 20 LEDs. Active
length is selected during initialization up to that chain's capacity. The cap
is an implementation/storage policy, not a universal WS2812 chain limit.

Color storage occupies `3 * sum(chain capacities)` bytes, plus per-chain
pointer/count/capacity/dirty state and compiler-managed scratch/stack. Only
configured transmitters consume flash. Check the complete application linker
map for SRAM and program use, especially when combining multiple chains and USB.
The default table uses `WS2812_MAX_LEDS` (20 unless overridden) for Main on RC5.
Explicit chain declarations replace that fallback; they need not have equal sizes.

Calculated wire time is 30-32 us per pixel. Show blocks for that interval plus
300 us latch, setup, and any interrupt work that runs after GIE restoration.

| LEDs | Data interval / approximate interrupt masking | Data plus latch, slower case |
|---:|---:|---:|
| 1 | 30-32 us | 332 us |
| 3 | 90-96 us | 396 us |
| 10 | 300-320 us | 620 us |
| 20 | 600-640 us | 940 us |

The table describes one chain and excludes function/setup overhead. Showing
several chains adds their blocking times and one latch interval per chain;
interrupts are restored between transmissions, subject to their entry state.
Separate chains do not latch simultaneously. It is not a guaranteed USB, encoder,
or button latency. During data, no ISR or main-loop task runs; during the blocking
latch, enabled interrupts can run but main-loop polling cannot. Update frequency
also matters: sending unchanged frames wastes the same blocking interval. Batch
changes and ShowIfDirty help avoid that work.

USB hardware buffering does not guarantee that arbitrarily delayed firmware
service is acceptable. Timer events may coalesce, and polled inputs can change
while the CPU is busy. Measure worst-case service latency in the complete
application. This package does not certify any LED count as USB-safe.

Other boundaries:

- All six PIC16(L)F1454/5/9 targets are accepted. Other PIC16 families require
  explicit port review and validation. Supported outputs remain RA4, RA5 and
  RC0-RC5 on every model; the additional 1459 GPIOs are not exposed by this backend.
- PWM2 is on RC6 on the 1459, so initializing RC3 there leaves PWM2 unchanged.
  RA4's analog selection is cleared on the 1455/1459; the 1454 has no ADC.
- Actual CPU clock must be 48 MHz, including after Sleep. A lower CPU divider
  changes pulse widths even when the USB module still has a 48 MHz clock.
- Supported pins are RA4, RA5 and RC0-RC5, one unique pin per chain (at most eight
  chains). RA4/RA5 must be released from oscillator functions by application
  configuration. RA3 is input-only and is rejected. The caller must release
  other peripheral overrides before initialization, including alternate mappings.
- No asynchronous or simultaneous multi-pin transmission, no animations or
  color-space conversion, no electrical acknowledgement, and no automatic power
  or USB lifecycle management are provided.
- Supply current, logic margins, wiring quality, and the real emitted waveform
  remain hardware responsibilities. Successful compilation does not validate them.

## Build and validate

### Native Unity tests

The first native configure downloads the pinned Unity archive and requires
network access. Reconfiguring reuses its build-tree download. For an existing
populated build, `-DFETCHCONTENT_FULLY_DISCONNECTED=ON` prevents fetching. For a
fresh offline build, supply an already extracted checkout of the pinned commit
with `-DFETCHCONTENT_SOURCE_DIR_UNITY=/path/to/Unity`; it must contain
`src/unity.c` and its accompanying headers/license. Source overrides bypass
archive verification, so use the same pinned revision. These are standard CMake
FetchContent controls. Native tests compile Unity with the project's test flags;
they do not add upstream's separate build/install configuration.


Use a native C compiler and CMake 3.20 or newer. Python is not needed for the
unit tests. For example, from a Visual Studio developer shell on Windows:

```powershell
cmake -S . -B build/host -DPIC16_RGB_BUILD_TESTS=ON
cmake --build build/host --config Debug
ctest --test-dir build/host -C Debug --output-on-failure
```

GCC and Clang work with the same commands on Linux/macOS. Ninja can be selected
with `-G Ninja`. A working compiler plus runtime/SDK is required; merely having
`cl.exe` without the Windows SDK/UCRT is insufficient. The test code is ordinary
C with `RUN_TEST`, named assertions, `setUp`, and `tearDown`; failure messages
report the C file and line directly.

CTest runs initialized and uninitialized suites at capacities 3 and 20. The
initialized suite covers all active lengths up to its capacity, indexed/bulk
edits, copied input lifetime, all byte-valued channels/brightness values,
rounding/clamping, readback, invalid/null/range errors, dirty tracking, fill,
clear, and immediate versus deferred transmissions. The uninitialized suite
runs in its own process so startup state is real rather than reset through
private test hooks. Tests use only the public API and observed port writes.

The fake frame port does not simulate GPIO or instruction timing. A fifth CTest
suite compiles the real PIC port C file with a test-only xc.h: it checks GPIO
setup, FSR/count setup, GIE masking/restoration for both entry states, and latch
ordering. That test substitutes only the assembly invocation and delay hooks
through the fake header. Actual pulse widths and electrical behavior still need
the separate XC8 HEX and hardware checks. A separate multi-chain suite checks
independent buffers, dirty flags, lengths, reinitialization and wire selection.
Configuration tests reject duplicate names/pins, unsupported outputs, empty
tables and invalid capacities. Consumer checks configure and compile the public
headers with exported include paths and definitions, while keeping tests off.

### XC8 example and emitted waveform

Tested toolchain: XC8 3.10 and PIC12-16F1xxx_DFP 1.10.270, with Python 3.10 for
these helper tools. Provide paths for your installation; no local absolute paths
are stored in the helper. Example PowerShell:

```powershell
$env:XC8_CC = 'C:/Program Files/Microchip/xc8/v3.10/bin/xc8-cc.exe'
$env:XC8_DFP = 'C:/Users/your-name/.mchp_packs/Microchip/PIC12-16F1xxx_DFP/1.10.270/xc8'
python tools/build_example.py --device 16F1454 --optimization 0
python tests/python/test_waveform.py build/16F1454-O0/example.hex
python tools/build_example.py --device 16F1455 --optimization 2
python tests/python/test_waveform.py build/16F1455-O2/example.hex
```

Use `--xc8` and `--dfp` instead of environment variables if preferred. The build
helper supports `-O0` and `-O2`. Do not claim an optimization mode was tested if
its license substituted another mode. Unused public API warnings in the minimal
example do not mean all API functions were linked into that example.

The HEX test remains Python because it is an instruction/data decoder, not a C
unit-test harness. It runs 3,081 data patterns, including every byte value in
one-/three-pixel frames and selected maximum-length frames. It follows the emitted
loop and checks bit values and inter-bit intervals, rejecting unexpected
instructions. It does not simulate startup, the full MCU, latch wall time, power,
or a USB host. Re-run after any compiler, port, or assembly change.

Before relying on hardware: measure RC5 pulses and reset low with an oscilloscope
or logic analyzer, test the first/last pixel and independent colors, verify all-off
before Sleep, and exercise inputs/USB under worst-case refresh load if integrated.

## Style and project status

Public contracts live in inc/ws2812.h; implementation constraints live beside the
assembly. C follows the Google C++ Style Guide where applicable, with the inherited
AGENTS.md taking precedence for four-space indentation, braces, comments, early
returns, static state, and validation. Target-dependent macros are retained where
they express the assembly sequence or compile-time configuration.

The source repository is [max-eelabs/PIC16-RGB](https://github.com/max-eelabs/PIC16-RGB),
with `main` as the default branch. See [CONTRIBUTING.MD](CONTRIBUTING.MD) for the
contribution workflow.

### Multi-pin validation

Native CTest includes cross-chain state/routing and every GPIO's setup isolation
for all six device-selection macros, GIE restoration, and invalid build
configurations. Build and check all supported
literal-pin transmitters with `tools/build_example.py --all-pins` using the same
`--xc8`, `--dfp`, `--device` and `--optimization` options shown above. Its output
is under `build/<device>-O<level>/all-pins/example.hex`; pass this file to
`tests/python/test_waveform.py`. Repeat at O0 and O2 for all six `--device`
values: `16F1454`, `16F1455`, `16F1459`, `16LF1454`, `16LF1455`, and `16LF1459`.
The waveform
interpreter checks every emitted transmitter, including all byte values at byte
boundaries, selected latch/bit identity and maximum-length frames.
These are instruction-level checks; verify actual waveforms and LED behavior
on hardware separately.

## License

Copyright 2026 Maxim Pavlov. Project-owned code and documentation are licensed
under the [Apache License, Version 2.0](LICENSE). Source, test and build files
carry Apache 2.0 headers and `SPDX-License-Identifier: Apache-2.0`, added with
[Google addlicense](https://github.com/google/addlicense).

Unity is a separate test dependency under its upstream MIT license; its source
and license are fetched into the build directory and are not relicensed here.
Generated build outputs and downloaded dependencies are excluded from header
updates. To add headers to new project source files, run from the repository root:

```sh
addlicense -l apache -s -c "Maxim Pavlov" -y 2026 -ignore "build/**" -ignore "**/_deps/**" -ignore "**/__pycache__/**" .
```

To verify headers without changing files:

```sh
addlicense -check -l apache -s -c "Maxim Pavlov" -y 2026 -ignore "build/**" -ignore "**/_deps/**" -ignore "**/__pycache__/**" .
```

## Continuous integration

The driver workflow in `.github/workflows/ci.yml` runs on pull request creation,
updates and reopening, every push (including accepted PR merges), and manual
`workflow_dispatch` runs. The jobs validate the standalone driver.

- `License`: verifies the canonical Apache 2.0 LICENSE and source headers using
  Google addlicense v1.2.0.
- `Native tests (gcc)` and `Native tests (clang)`: configure, compile and run
  the Unity/CTest suites on Ubuntu 24.04, including downstream CMake checks.
- `XC8 (O0)` and `XC8 (O2)`: build the standalone example and all-pin fixture
  for PIC16F1454 only and check their emitted HEX waveforms. Other supported
  devices can be validated locally using the build helper.

Actions are pinned to commit SHAs. The Linux toolchain setup in
`.github/scripts/setup-xc8.sh` uses XC8 3.10 in FreeMode and device pack
1.10.270 from Microchip, with both downloads verified against pinned SHA256
checksums. It caches the installed toolchain between runs. Native test logs
and XC8 HEX/ELF/map files are uploaded as artifacts for 14 days.

The workflow uses read-only repository permissions and no secrets, allowing
fork pull requests. These job names can be selected as required status checks
in GitHub branch rules. Checks become active when these files are pushed to
the driver's GitHub repository. Hardware validation remains separate from CI.
