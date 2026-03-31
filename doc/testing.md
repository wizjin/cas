# Testing Rules

Back to `doc/index.md`.

## Unit Testing

- Use `cmocka`.
- Prefer mocks over dependence on external services, devices, or system state.
- Keep tests deterministic.

## Coverage

- Treat `100%` coverage as the target for unit-tested code.
- Use coverage results to find untested branches and error paths.

## Static Analysis

- Run `clang-tidy` only on files under `src/` and `include/`.
- Exclude tests and third-party code from static analysis scope.

## Verification Focus

- Test public behavior, failure paths, and boundary conditions.
- Add mocks for external interactions instead of coupling tests to the environment.
