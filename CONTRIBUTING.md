# Contributing to SHiP Geometry Service

Thank you for your interest in contributing! As part of the SHiP Collaboration, we follow a set of standards to ensure code quality and maintainability.

## Development Workflow

1. **Fork and Clone**: Create a fork of the repository and clone it locally.
2. **Environment**: Install [pixi](https://pixi.sh) — it provisions all build dependencies (GeoModel 6.22+, Geant4 11.x, SHiPGeometry) from `conda-forge` and `prefix.dev/ship`. Manual installs are also supported (see the README).
3. **Pre-commit Hooks**: We use [`prek`](https://github.com/j178/prek) (a drop-in `pre-commit` replacement) to enforce coding standards. The hook tools come from the pixi `lint` environment, so versions are tracked in `pixi.lock` and run identically everywhere. Install the hooks once:
   ```bash
   pixi run install-hooks
   ```
   Run all hooks manually at any time with `pixi run lint`.
4. **Branching**: Create a feature branch for your changes.
5. **Coding Standards**:
   - Follow the existing C++ style (enforced by `clang-format` and `cpplint`).
   - CMake formatting is enforced by `gersemi`.
   - Ensure all files have the correct SPDX license headers (REUSE compliant).
6. **Commits**: We follow [Conventional Commits](https://www.conventionalcommits.org/). This helps in automated changelog generation.
   - `feat: ...` for new features
   - `fix: ...` for bug fixes
   - `docs: ...` for documentation changes
   - `style: ...` for formatting
   - `refactor: ...` for code refactoring
7. **Testing**:
   - Build and run the tests:
     ```bash
     pixi run test
     ```
     During iteration, `pixi run build` rebuilds incrementally without re-running ctest.
   - Add tests for new features.
8. **Submission**: Open a Pull Request against the `main` branch. Ensure the CI passes.

## Coding Style

- **C++**: We use C++20. Style is defined in `.clang-format`.
- **CMake**: Formatting enforced by `gersemi`.

## Licensing

This project is licensed under the **LGPL-3.0-or-later**. All contributions must be compatible with this license. Each new file must include an SPDX identifier and copyright notice.
