# CAS

CAS is an AI agent system implemented in C.

## Overview

The project targets a modular, low-coupling architecture for LLM-driven agent workflows. The repository uses CMake for builds and a root `Makefile` for common developer commands.

## Prerequisites

- C17-compatible compiler
- CMake
- Make
- clang-format
- clang-tidy
- cmocka

## Build And Development

Use the root `Makefile` as the main entrypoint:

```sh
make clean
make build
make format
make tidy
make test
make coverage
make release
```

Release artifacts are expected under `build/release/`.

## Dependencies

The project prefers system-installed third-party libraries. If a required dependency is unavailable, CMake may fetch it with `FetchContent` and store the fetched source under `libs/`.

## Additional Documentation

Repository automation and LLM-oriented rules are defined in `AGENTS.md` and the files under `doc/`.

## License

Distributed under the MIT License. See [`LICENSE`](LICENSE) for details.
