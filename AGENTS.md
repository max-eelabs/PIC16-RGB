# Code Style

These conventions apply to project-owned C code in `src/`, `inc/`, `examples/`,
and `tests/`. Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
where its guidance applies to C. The rules below, existing API names, and XC8
constraints take precedence. Preserve behavior when making style changes, and
leave generated files and downloaded dependencies untouched.

## Formatting and Naming

- Use four-space indentation and put opening braces on the declaration or
  control-flow line.
- Follow the existing names: `Ws2812SetPixel` for functions, `Ws2812Color` for
  types, `led_count` for variables, `kWs2812ChainMain` for constants, and
  `WS2812_CHAINS` for macros.
- Separate logical steps with one blank line. Keep related register writes
  together; do not space out every statement.
- Put introductory comments directly above the code they describe, with the
  separating blank line before the comment.
- Separate individually documented file-scope declarations and distinct switch
  cases with a blank line.
- Avoid blank lines immediately inside function braces and trailing whitespace.

## Headers and State

- Keep public declarations in `inc/` and private implementation headers in `src/`.
- Make headers self-contained, with include guards and explicit dependencies.
  Include a source file's corresponding header first.
- Keep module-private functions and state `static`. Use `const` for values that
  do not change, and declare local variables near their first use.
- Prefer typed functions and constants over macros when practical. Retain macros
  needed for chain declarations, hardware registers, and timed assembly.
- Keep code compatible with C99 and the supported XC8 toolchain.

## Comments and Contracts

- Use `//` for single-line comments, including descriptions of constants,
  variables, and tables.
- Use multiline `/** ... */` blocks for file overviews and function contracts.
  Use `@file`, `@brief`, `@param`, `@return`, and `@note` where useful; omit
  empty tags and descriptions that only repeat a name.
- Keep public API contracts in headers and implementation explanations beside
  the relevant code.
- Document buffer ownership and lifetime, state changes, calling context,
  preconditions, and return-value meaning where applicable.
- Explain intent and constraints, especially instruction timing and register
  ownership. Update comments when behavior changes.

## Control Flow

Use early returns for invalid inputs and unmet preconditions. Keep the normal
path at the outer indentation level, separated from guards by a blank line.
Do not add `else` after a branch that returns. Preserve short-circuit evaluation
and required cleanup.

```c
bool UpdateLeds(bool ready) {
    if (!ready) {
        return false;
    }

    return Ws2812ShowIfDirty(kWs2812ChainMain);
}
```

## Validation

For code changes, run the relevant host tests. For changes affecting target code
or timing, also build with XC8 and check the emitted HEX waveform for each
supported device and optimization setting affected. Instructions are in
[tests/README.md](tests/README.md). Check the diff for whitespace errors, and
report hardware validation separately from compilation and automated tests.
