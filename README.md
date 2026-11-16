# PIC16-RGB

A C driver for WS2812B-V6 addressable RGB LEDs on PIC16 microcontrollers.
Set individual pixel colors and brightness, fill a strip, or prepare several
changes and display them together. Multiple LED chains can use separate GPIOs,
with independent color buffers and lengths.

The driver uses the 24-bit GRB format used by many RGB
[NeoPixel products](https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use).
Its timing targets WS2812B-V6; check the timing requirements of other NeoPixel
revisions. RGBW and 400 kHz pixels are not supported.

## Supported Hardware

| Requirement | Supported configuration |
|---|---|
| MCUs | PIC16F1454, PIC16F1455, PIC16F1459, PIC16LF1454, PIC16LF1455, PIC16LF1459 |
| Compiler | MPLAB XC8 (tested with 3.10) |
| CPU clock | 48 MHz |
| Data outputs | RA4, RA5, RC0–RC5 |
| Chain capacity | 1–20 LEDs per GPIO, selected at build time |
| Default configuration | One chain named `Main` on RC5, with capacity for 20 LEDs |

The transmitter uses timed assembly and static memory. It needs no timer, SPI,
PWM, DMA, or heap. Sending a frame blocks the caller and briefly disables
interrupts; see [Timing and Memory](#timing-and-memory).

## Add the Driver to Your Project

### Direct Source Inclusion

1. Add `src/ws2812.c` and `src/ws2812_pic16.c` to your firmware build.
2. Add `inc/` to the compiler's include path.
3. Select your exact MCU in XC8 and include `ws2812.h` in your application.

This uses the default `Main` chain on RC5. To reduce its capacity, define
`WS2812_MAX_LEDS` to a value from 1 to 20 in the build settings for **all** source
files. The driver itself has no external library dependencies.

### CMake

With CMake 3.20 or newer, add this repository as a subdirectory and link the
provided target to your firmware target:

```cmake
set(PIC16_RGB_CHAINS "Main:RC5:3" CACHE STRING "LED chains")
add_subdirectory(path/to/PIC16-RGB)
target_link_libraries(my_firmware PRIVATE PIC16RGB::ws2812)
```

The target adds the driver sources, headers, and chain configuration to
`my_firmware`. Your firmware target supplies the XC8 device, device pack, and
compiler settings. Native tests are off by default and are built separately.

## Use the Driver

Configure a stable 48 MHz CPU clock before calling the driver. Defining
`_XTAL_FREQ` alone does not configure the oscillator. The selected pins must be
available as GPIOs, including the oscillator pins when using RA4 or RA5.
[examples/main.c](examples/main.c) includes clock startup and device configuration
for a complete three-pixel example.

Call this function after clock startup to display red, green, and blue at 30%
brightness:

```c
#include "ws2812.h"

bool InitStatusLeds(void) {
    // Initialization also sends black to all three LEDs.
    if (!Ws2812Init(kWs2812ChainMain, 3)) {
        return false;
    }

    const Ws2812Color colors[] = {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255}
    };
    if (!Ws2812SetPixels(kWs2812ChainMain, 0, colors, 3, 30)) {
        return false;
    }

    return Ws2812Show(kWs2812ChainMain);
}
```

Pixel indexes start at zero, nearest the PIC. Colors use RGB channel values from
0 to 255. Brightness is a percentage from 0 to 100; higher values are clamped to
100. Scaling happens when you assign a color, without gamma correction. The
buffer stores the scaled result, so changing brightness requires assigning the
color again.

For a single immediate change, use `Ws2812SetColor()`:

```c
// Change pixel 1 to green at 30%, preserving the other stored colors.
Ws2812SetColor(kWs2812ChainMain, 1, 0, 255, 0, 30);
```

For several changes, update the buffer first and send it once:

```c
const Ws2812Color blue = {0, 0, 255};
const Ws2812Color red = {255, 0, 0};
Ws2812Fill(kWs2812ChainMain, blue, 20);
Ws2812SetPixel(kWs2812ChainMain, 0, red, 50);
Ws2812ShowIfDirty(kWs2812ChainMain);

// When needed, turn off every active LED immediately.
Ws2812Clear(kWs2812ChainMain);
```

| Function | Purpose |
|---|---|
| `Ws2812Init` | Set the active length, configure the GPIO, and clear the LEDs |
| `Ws2812SetPixel`, `Ws2812SetPixels`, `Ws2812Fill` | Change buffered colors without transmitting |
| `Ws2812Show` | Send the full active chain |
| `Ws2812ShowIfDirty` | Send only if buffered colors have changed |
| `Ws2812SetColor` | Set one pixel and send the full chain immediately |
| `Ws2812Clear` | Set all active pixels to black and send immediately |
| `Ws2812GetPixel` | Read a stored, brightness-scaled color |
| `Ws2812Count`, `Ws2812IsDirty` | Read the active length or pending-change state |

Every call takes a chain identifier. Call the API from the main program, not an
interrupt handler; it is not reentrant, even across chains. Input colors are
copied, so the driver does not retain your arrays. Operation return values report
local success, not an acknowledgement from the LEDs. Full contracts and error
conditions are in [inc/ws2812.h](inc/ws2812.h).

### Multiple Chains

Give each chain a unique name and GPIO, with its own capacity. For CMake, set the
chain list before `add_subdirectory()`:

```cmake
set(PIC16_RGB_CHAINS "Main:RC5:3;Aux:RC2:5" CACHE STRING "LED chains")
```

To change an existing CMake build, pass
`-DPIC16_RGB_CHAINS="Main:RC5:3;Aux:RC2:5"` when configuring.

For direct source inclusion, create `led_chains.h`:

```c
#ifndef LED_CHAINS_H_
#define LED_CHAINS_H_

#define WS2812_CHAINS(X) \
    X(Main, RC5, 3) \
    X(Aux, RC2, 5)

#endif
```

Add that header's directory to the include path and define
`WS2812_CONFIG_HEADER="led_chains.h"` for all source files, preserving the quotes
in the macro value. This replaces the default chain configuration.

Names become identifiers such as `kWs2812ChainMain` and `kWs2812ChainAux`.
Initialize each chain before using it:

```c
if (!Ws2812Init(kWs2812ChainMain, 3) ||
    !Ws2812Init(kWs2812ChainAux, 5)) {
    return false;
}

Ws2812SetColor(kWs2812ChainAux, 4, 0, 0, 255, 50);
```

Showing one chain leaves the others unchanged. Chains transmit sequentially and
do not latch simultaneously. Before shortening an initialized chain, clear its
old length so that trailing LEDs do not retain their previous colors.

## Wiring and Protocol

```text
PIC GPIO --> LED 0 DIN
             LED 0 DOUT --> LED 1 DIN
                           LED 1 DOUT --> LED 2 DIN

LED supply + --> VDD on every LED
Common ground --> VSS on every LED and the PIC
```

Power the LEDs from a supply sized for their load and share ground with the PIC.
The GPIO carries only data. Check the MCU package pinout and voltage limits, and
use a level shifter if the GPIO voltage does not meet the LED's input threshold.
The F and LF variants have different supply requirements; consult the
[PIC16(L)F1454/5/9 datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/PIC16LF1454-5-9-14-20-Pin-8-Bit-Flash-USB-Microcontroller-with-XLP-Technology-DS40001639C.pdf)
for your device at 48 MHz.

Each LED consumes 24 bits in green, red, blue order, most-significant bit first,
and forwards the remaining data to the next LED. A shorter high pulse represents
zero; a longer one represents one. After the frame, the driver holds data low
for 300 µs to latch the colors. LEDs retain those colors until the next frame.

Changing one pixel still sends every active pixel in that chain. Buffered
updates let several changes appear together without sending intermediate frames.

## Timing and Memory

At 48 MHz, the assembly transmitter produces these nominal pulse widths:

| Bit | High | Low until next bit | Total |
|---|---:|---:|---:|
| 0 | 333 ns | 917 ns | 1.250 µs |
| 1 | 667 ns | 667 ns | 1.333 µs |

The final bit is followed by the latch interval. The timed loop is in
[src/ws2812_pic16.c](src/ws2812_pic16.c); it is designed for the
[WS2812B-V6 timing specification](https://datasheet.lcsc.com/datasheet/pdf/0689d8fd6dabfc7959e82552d4ffad8b.pdf?productCode=C52917433).

Interrupts are disabled during data transmission, about 30–32 µs per LED.
The previous interrupt-enable state is restored before the 300 µs latch wait.
For 20 LEDs, a call blocks for about 900–940 µs, plus setup and any interrupt
handling during the latch. Account for this delay when servicing USB, timers,
or polled inputs. Interrupt handlers must not touch the driver-owned GPIOs or
change the clock during a transfer.

Color buffers use three bytes per configured LED, plus per-chain state and
compiler overhead. The 20-LED limit per chain is a driver limit, not a protocol
limit. Check your application's linker map when allocating several chains.

## Build and Validate

To build the standalone example, install XC8 and the PIC12-16F1xxx device family
pack. The tested versions are XC8 3.10 and DFP 1.10.270. Set `XC8_CC` to the
compiler executable and `XC8_DFP` to the pack's `xc8` directory, or pass their
paths with `--xc8` and `--dfp`:

```sh
python tools/build_example.py --device 16F1454 --optimization 0
python tests/python/test_waveform.py build/16F1454-O0/example.hex
```

The helper accepts all six supported devices and optimization levels `0` and
`2`. Python 3.10 or newer is required. Use `--all-pins` to build the waveform
fixture for every supported GPIO; that fixture is for analysis, not flashing.

Run the native tests with CMake and a host C compiler:

```sh
cmake -S . -B build/native -DPIC16_RGB_BUILD_TESTS=ON
cmake --build build/native --config Debug
ctest --test-dir build/native -C Debug --output-on-failure
```

The first configure downloads the pinned Unity test dependency. For an offline
build, provide an existing Unity source directory with
`-DFETCHCONTENT_SOURCE_DIR_UNITY=/path/to/unity`.

See [tests/README.md](tests/README.md) for test coverage and the target matrix.
Native tests check API behavior; the HEX checker checks emitted instructions and
pulse timing. Actual electrical timing and LED operation need separate hardware
validation with an oscilloscope or logic analyzer.

## Contributing

See [CONTRIBUTING.MD](CONTRIBUTING.MD) for the contribution workflow and
[AGENTS.md](AGENTS.md) for code style. Public headers are in `inc/`; implementation
and private headers are in `src/`.

## License

Copyright 2026 Maxim Pavlov. Code and documentation are licensed under the
[Apache License, Version 2.0](LICENSE). Unity is a separate test dependency under
its upstream MIT license.
