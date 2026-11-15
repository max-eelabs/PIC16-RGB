# Tests

The test root contains only this overview and the CMake entry point. Test code
is grouped by runner, while shared target configuration stays in fixtures/:

- `unity/`: C suites for the public API, chain isolation, startup state, and GPIO
  backend behavior for all six PIC16(L)F1454/5/9 targets. Its `fakes/` directory
  holds native hardware substitutes.
  Unity itself is fetched into the build tree through `cmake/Unity.cmake`.
- `python/`: the emitted-HEX waveform interpreter; it does not rewrite C code.
- `cmake/`: CTest checks for invalid configurations and dependency consumption.
  `consumer/` is a minimal downstream project with a public-header compile probe.
- `fixtures/`: chain declaration headers shared by native and XC8 checks, plus
  the build-only entry point that keeps every GPIO transmitter in the test HEX.

Run native tests from the repository root with a working host C compiler:

```sh
cmake -S . -B build/native -DPIC16_RGB_BUILD_TESTS=ON
cmake --build build/native --config Debug
ctest --test-dir build/native -C Debug --output-on-failure
```

The first configure fetches the pinned Unity archive. See the root README for
FetchContent cache/offline controls. Firmware dependency builds leave tests off
and do not fetch Unity.

Build the XC8 waveform fixture with `tools/build_example.py --all-pins`, supplying
`--xc8`, `--dfp`, `--device` and `--optimization` as described in the root README.
Run both optimization levels (0 and 2) for 16F1454, 16F1455, 16F1459,
16LF1454, 16LF1455, and 16LF1459. Then check its output, for example:

```sh
python tests/python/test_waveform.py build/16F1454-O0/all-pins/example.hex
```

The fixture is for build/instruction checks, not flashing. Physical clock startup
is demonstrated in `examples/main.c`. Host tests and instruction analysis do not
establish electrical timing or correct LED behavior on hardware.
