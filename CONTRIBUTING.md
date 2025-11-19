# Contributing to VersionForge ✍️

Thank you for your interest in contributing to VersionForge. This document describes the preferred workflow, code style guidance, and expectations for contributions.

## Where to start 🧭

- Check open issues for small tasks or feature requests.
- If you'd like to propose a larger design change, open an issue first to discuss the approach.

## Workflow 🔁

1. Fork the repository on GitHub.
2. Create a branch for your work:

   ```bash
   git checkout -b feature/your-feature
   ```

3. Make commits that are focused and atomic. Use descriptive commit messages.
4. Run `make` to build the project and verify nothing breaks.
5. Add tests if you introduce new behavior (unit/integration tests are valued).
6. Push your branch to your fork and open a Pull Request against `main` in the upstream repository.

## Pull Request guidelines 🧾

- Provide a clear title and description for the PR.
- Explain the motivation and the change at a high level.
- Link any related issues.
- If the change touches multiple subsystems, explain how you verified the integration.
- Keep PRs small and focused where possible.

## Coding style 🧹

- Follow the existing code style in `src/` and `include/`. Keep formatting consistent.
- Use descriptive names (avoid single-letter variables except for short loop indices).
- Prefer clarity over cleverness; aim for readable, maintainable C code.
- No trailing whitespace; keep line lengths reasonable (~80-100 chars when possible).

## Testing 🧪

- Build locally: `make`.
- Manually exercise commands used by your change (e.g., `init`, `commit`, `push`, `pull`).
- If adding new CLI behavior, document usage in `README.md` or tests.

## Communication 💬

- Open an issue to discuss larger changes before starting work.
- Use clear PR descriptions and respond to review feedback promptly.

## Code of Conduct 🤝

Please be respectful and constructive in comments, PRs, and issues. If you encounter problematic behavior, notify the repository maintainers.

## Attribution 🏷️

When adding your name to the project (e.g., in the README Authors list), follow the project's license and sign your contribution via the standard GitHub mechanisms (your GitHub account and PR).

---

Thank you — the maintainers appreciate your help. If you want, add your name to the `README.md` Authors section once your PR is merged.
