# Style Rules

Back to `doc/index.md`.

## Language

- Use English for documentation, comments, and logs.
- Do not place sensitive information in code, comments, docs, logs, or commit messages.
- Do not include passwords, API keys, tokens, or absolute paths containing usernames.

## C Rules

- Follow the C17 standard.
- Follow K&R style.
- Keep formatting compatible with `clang-format`.
- Keep code clean under `clang-tidy`.
- Put public headers in `include/`.
- Put internal headers in `src/`.
- Do not hide pointer semantics in typedef aliases.
- Prefer declaring local variables with initialization at first use.
- Do not group local declarations at function entry.
- Merge declaration and first assignment whenever possible. Keep split declaration and assignment only when required by C APIs such as `va_list`.
- Treat a moved declaration as complete only when the declaration site is also the first meaningful assignment site. Do not claim a declaration was moved down if it still appears earlier with placeholder initialization such as `NULL`, `0`, or `false`.
- When cleanup paths need a variable earlier than its first successful value, restructure the control flow or cleanup logic instead of keeping a placeholder-initialized declaration near the top of the function.
- Place feature-test macros through CMake target definitions instead of source-file preprocessor blocks when possible.
- Add a compiler-format attribute wrapper on printf-style variadic APIs so static analyzers can validate format and `va_list` usage.
- Do not use `goto`.
- Keep shared internal configuration macros in `src/cas_cfg.h`.

## Naming

- Use `CAS_` or `cas_` prefixes for code files, functions, macros, and related symbols when applicable.
- Prefer self-explanatory names over comments.
- Use `CAS_LOG_BUF_SIZE` as the log message buffer size macro.
- Normalize log category names to exactly four characters in `cas_log_get_category()`: truncate longer names and pad shorter names with spaces. Keep `cas_log_format_prefix()` focused on fixed-width formatting only.

## Comments And Design

- Avoid comments unless they remove real ambiguity.
- Keep modules focused on a single responsibility.
- Favor low coupling and high cohesion.
- Preserve extensibility through stable interfaces and small module boundaries.
- Inline trivial single-use conditions instead of adding tiny helper functions.
- Prefer single unlock paths inside functions that manage mutexes.
- Prefer the simplest locking model that satisfies the requirement. Avoid adding lifecycle counters, condition variables, or similar coordination unless the requirement clearly needs them.
