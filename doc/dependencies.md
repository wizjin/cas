# Dependency Rules

Back to `doc/index.md`.

## Selection Policy

- Prefer system-installed third-party libraries.
- Use CMake `FetchContent` only when a required dependency is unavailable from the system.

## Storage Policy

- Store fetched third-party sources under `libs/`.
- Keep third-party code isolated from project source code.

## Integration Policy

- Manage dependency configuration from the root `CMakeLists.txt`.
- Keep dependency usage explicit and minimal.
- Avoid introducing dependencies that are not required by a clear project need.
