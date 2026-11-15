# Firmware documentation and code style

These rules apply to application-owned code in this firmware directory,
especially `main.c` and `src/`. Preserve existing behavior when changing style.
Do not reformat downloaded MLA sources or generated build files.

## Documentation

- Use `//` for single-line comments, including brief descriptions of constants,
  variables and tables. Do not use single-line `/** ... */` comments.
- Use multiline Javadoc/Doxygen-style `/** ... */` blocks for file overviews
  and function contracts. Use `@file`, `@brief`, `@param`, `@return` and `@note`
  where relevant; do not add empty or redundant tags.
- Document buffer ownership and lifetime, state changes, polling/interrupt
  context, preconditions, and return-value meaning where applicable.
- Keep public API contracts in headers and implementation-specific explanations
  beside the code. Document MLA callback contracts at their definitions.
- Explain intent and non-obvious constraints rather than narrating each statement.
  Keep comments accurate when code changes.

## Code blocks and formatting

- Use four-space indentation and opening braces on the declaration/control-flow
  line. Preserve established names and required MLA symbols.
- Group related statements into logical blocks separated by one blank line,
  following `main.c` and `src/usb/usb_hid.c`.
- Separate setup, readiness checks, transfer work and final returns when they
  form distinct steps. Keep tightly related register writes together.
- Keep a block's introductory comment directly above its first statement;
  place the separating blank line before the comment, not after it.
- Separate individually documented file-scope declarations and distinct switch
  cases with a blank line. Do not insert blank lines between every statement.
- Avoid blank lines immediately inside function braces and trailing whitespace.
- Keep module-private functions and state `static`. Use self-contained headers
  with include guards and explicit dependencies.

## Early returns

- If an entire function body, or its remaining useful work, is wrapped in an
  `if`, prefer an inverted guard followed by an early return. Leave the normal
  work at the function's outer indentation level.
- Separate the guard from the normal work with a blank line. Do not add an
  `else` after a branch that returns.
- Preserve short-circuit evaluation, return values and required cleanup. Do
  not apply the pattern if returning early would skip necessary work.

```c
void HandleRequest(void) {
    if (!request_supported) {
        return;
    }

    PrepareReply();
    SendReply();
}
```

## Build integration and validation

- Add new application `.c` files to `cmake/PICnUSB/default/user.cmake`.
- Keep custom build changes in `user.cmake`, not MPLAB-generated files.
- Build with the configured XC8 toolchain after code/control-flow changes and
  check the diff for whitespace errors. Report hardware validation separately
  from compilation; a successful build does not establish USB runtime behavior.

## Standalone PIC16-RGB application of these rules

This file preserves the source firmware project's rules above. In this standalone
library, apply its documentation, four-space formatting, static-state, early-return,
and validation rules to src/, inc/, examples/, and tests/. Follow the Google
C++ Style Guide where applicable to conventional C; these explicit project rules
and XC8 constraints take precedence, including indentation and assembly macros.

There is no MPLAB-generated CMake project here. The upstream user.cmake source-list
rule applies when integrating into that firmware, not when building this example.
Keep native tests in CMake/CTest and standalone XC8 commands in
tools/build_example.py. Unit tests use Unity with a link-time hardware fake; do
not concatenate or rewrite production C in a Python harness. Keep private port
contracts in src/ and public API headers in inc/, following MLA layout. Fetch
Unity only for native tests through cmake/Unity.cmake, pinned by immutable commit
and archive SHA256. Do not vendor Unity or modify downloaded sources/license.
Keep Unity suites and fakes in tests/unity/, Python checks in tests/python/,
CMake integration checks in tests/cmake/, and shared inputs in tests/fixtures/.
Run host API tests, build with XC8, and check the emitted HEX waveform for each supported target and
optimization setting touched by a change. Keep hardware validation separate.
Do not initialize Git or configure a remote unless the user requests it.
