# CLI Rules

Back to `doc/index.md`.

## Commands

- `cas help` prints command usage and the available subcommands.
- `cas version` prints the project version string.
- Running `cas` without a subcommand prints the same help text.

## Implementation

- Keep subcommand dispatch in `src/cas_cli.h` and `src/cas_cli.c`.
- Keep configuration macros in `src/cas_config.h`.
- Use `CAS_VERSION` as the version macro.
- Use a command table with structs and function pointers for dispatch.
- Keep command output deterministic for unit tests.
