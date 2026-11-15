# Contributing

Thank you for your interest in contributing to PIC16-RGB! Bug reports, driver
improvements, tests, and documentation updates are welcome.

Keep each contribution focused on one change so it is easy to review. Explain
the problem you are solving and the resulting behavior in your pull request.

- Create a `feature/{heading}` branch with a short, descriptive heading, such as
  `feature/add-patterns`.
- You can find  code style and documentation conventions in [AGENTS.md](AGENTS.md)
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

Run the applicable checks described in [Build and validate](README.md#build-and-validate)
before opening your PR. For timing or GPIO changes, include the XC8 build and
waveform-check results, and state whether you tested on hardware. Compilation
and host tests alone do not establish correct electrical behavior.

Keep Apache 2.0 license headers on new source files and preserve third-party
license notices. See the [license instructions](README.md#license) for the
`addlicense` commands.
