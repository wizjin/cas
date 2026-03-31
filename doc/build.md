# Build Rules

Back to `doc/index.md`.

## Build System

- Use CMake.
- Keep build configuration centralized in the root `CMakeLists.txt`.
- Use the root `Makefile` as the developer-facing command entrypoint.

## Supported Commands

- `make clean`
- `make build`
- `make format`
- `make tidy`
- `make test`
- `make coverage`
- `make release`

## Output Policy

- Do not run Make in parallel.
- Keep Make output quiet.
- Print only essential progress lines with `echo`.
- Keep all build artifacts and intermediate files under `build/`.
- Put release outputs under `build/release/`.
- Do not leave temporary build files in the repository root.
