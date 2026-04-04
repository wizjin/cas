# CLI Rules

Back to `doc/index.md`.

## Commands

- `cas help` prints command usage and the available subcommands.
- `cas --help` prints the same help text as `cas help`.
- `cas --version` prints the project version string.
- `cas version` prints detailed build information.
- `cas start` starts the CAS service.
- `cas status` prints the CAS service status.
- `cas stop` stops the CAS service.
- Running `cas` without a subcommand prints the same help text.

## Implementation

- Keep subcommand dispatch in `src/cas_cli.h` and `src/cas_cli.c`.
- Keep configuration macros in `src/cas_cfg.h`.
- Use `CAS_VERSION` as the version macro.
- Use `CAS_GIT_COMMIT` and `CAS_BUILD_DATETIME` for version output build metadata.
- Keep the help output command list aligned with the command table.
- Use a command table with structs and function pointers for dispatch.
- Keep command output deterministic for unit tests.
