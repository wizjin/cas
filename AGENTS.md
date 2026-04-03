## Communication
- Fully utilize context to avoid token exhaustion.
- Always respond in Simplified Chinese.
- Answer directly without pleasantries.

## Execution
- Read relevant files before making changes.
- Ask when uncertain, don't guess.
- Avoid unnecessary comments.

## Security
- Never commit secrets, keys, or credentials to repositories.
- Never expose sensitive information in code or logs.

## Extensibility
- Rules below this section may be added, edited, or deleted.
- This section and everything above it requires user confirmation to change.

## Repository Role
- `AGENTS.md` and files under `doc/` are for LLMs.
- `README.md` is for humans only. Keep it concise and user-oriented. Focus on what CAS is, how to install or build it, and how to use it. Avoid LLM-facing, repository-internal, or overly detailed developer workflow content.
- Keep LLM-facing documents short, explicit, and task-oriented.
- Prefer updating, merging, or replacing existing content instead of appending new sections.

## Document Layout
- Start with `doc/index.md` to find task-specific documents.
- Read only the minimum set of files required for the current task.
- Keep one canonical source for each rule. Link to it instead of duplicating content.
- Split documents when a file starts mixing unrelated concerns or becomes longer than needed for focused LLM use.

## Language And Content
- Write documentation, comments, logs, and commit messages in English.
- Do not include sensitive information in documentation, comments, logs, commit messages, or code.
- Do not include passwords, API keys, tokens, private endpoints, or absolute paths containing usernames.

## Git Workflow
- Use Git for source control.
- Follow `Conventional Commits` for commit messages.
- Do not rewrite history unless the user explicitly requests it.

## Code Rules
- Target the C17 standard.
- Follow K&R formatting and keep code compatible with `clang-format`.
- Use `clang-tidy` for static analysis.
- Put public headers in `include/`.
- Put internal headers in `src/`.
- Use `CAS_` or `cas_` as the prefix for code files, functions, macros, and related symbols when applicable.
- Prefer struct typedef aliases, but do not hide pointer semantics in them. Use explicit `*` in APIs.
- Keep project configuration macros in `src/cas_config.h` when they are consumed by internal source files.
- Prefer self-explanatory names and avoid comments unless they prevent real ambiguity.
- Prefer short, conventional C local variable names only when they remain self-explanatory in context.
- Prefer declaration with initialization at first use. Keep split declaration and assignment only when required by C APIs such as `va_list`.
- Inline trivial one-use conditions instead of creating tiny helper functions that obscure the main path.
- Design interfaces and modules with single responsibility, low coupling, high cohesion, open/closed principle, and Law of Demeter.
- When reviewing naming, focus refactors on production and shared interfaces first; keep test case names descriptive unless the user explicitly asks to shorten them.
- When the user narrows review scope to one issue class, keep the audit and plan limited to that class unless the user expands scope.

## Build And Dependency Rules
- Use CMake.
- Keep build configuration centralized in the root `CMakeLists.txt`.
- Use the root `Makefile` as the public entrypoint for common developer tasks.
- Support `make clean`, `make build`, `make format`, `make tidy`, `make test`, `make coverage`, and `make release`.
- Apply feature-test macros needed by system APIs through CMake `target_compile_definitions` with the narrowest target scope possible, and verify declaration visibility before replacing project wrappers with direct system calls or macros.
- Keep Make output quiet. Print only essential status lines with `echo`.
- Do not run Make in parallel.
- Keep all build artifacts and intermediate files under `build/`.
- Put release outputs under `build/release/`.
- Use separate build directories for configurations that change instrumentation or dependency build state, such as coverage versus normal debug builds.
- Prefer system libraries first. Use CMake `FetchContent` only when a required dependency is unavailable, store fetched sources under `libs/`, and keep `FetchContent` build and subbuild state under `build/`.
- Suppress third-party CMake warnings from top-level build entrypoints instead of patching fetched sources.

## Quality Rules
- Restrict static analysis scope to `src/` and `include/`.
- Ignore tests and third-party code during static analysis.
- Use `cmocka` for unit tests.
- Name unit test source files as `tests/test_<module>.c`.
- Prefer mocks over reliance on external environments.
- Prefer test-only build configuration or test translation units over exposing production-only test hooks when covering internal branches.
- Move reusable helper logic into focused utility modules instead of keeping module-specific copies of generic helpers.
- Treat `100%` coverage as the target for unit-tested code.
- Re-run tests after changes to locking, object lifetime, or other concurrency-sensitive control flow.
- Keep thread-safety scope explicit. Protect shared mutation with simple locking, and do not infer thread-safe destruction or lifetime management unless the requirement states it.

## Navigation
- Read `doc/index.md` first for documentation routing.
- Read `doc/build.md` for build and dependency workflow.
- Read `doc/style.md` for code style and naming rules.
- Read `doc/testing.md` for testing and analysis expectations.
- Read `doc/dependencies.md` for third-party dependency policy.
