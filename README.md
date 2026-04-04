# CAS

CAS is an AI agent system implemented in C.

## Install

Install a packaged binary when available. If no package is available for your platform, build CAS from source.

## Build From Source

- C17-compatible compiler
- CMake 3.21 or newer
- Make

Dependencies:

- `jemalloc` when available
- `libuv` 1.51.0
- `llhttp` 9.3.0
- `cJSON` 1.7.19

CAS prefers system libraries. Missing `libuv`, `llhttp`, or `cJSON` will be fetched automatically by CMake into `libs/`.

Build:

```sh
make build
make test
make release
```

- debug build: `build/debug/`
- release build: `build/release/`

## Run CAS

After installation or build:

```sh
cas help
cas --version
cas version
```

- `cas --version` prints the short semantic version such as `0.1.0`
- `cas version` prints build information

Other commands:

```sh
make clean
make format
make tidy
make coverage
```

## License

Distributed under the MIT License. See [`LICENSE`](LICENSE) for details.
