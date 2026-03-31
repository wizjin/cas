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

## Naming

- Use `CAS_` or `cas_` prefixes for code files, functions, macros, and related symbols when applicable.
- Prefer self-explanatory names over comments.

## Comments And Design

- Avoid comments unless they remove real ambiguity.
- Keep modules focused on a single responsibility.
- Favor low coupling and high cohesion.
- Preserve extensibility through stable interfaces and small module boundaries.
