# Contributing to SHiP Geometry Service

Thank you for your interest in contributing! As part of the SHiP Collaboration, we follow a set of standards to ensure code quality and maintainability.

## Development Workflow

1. **Fork and Clone**: Create a fork of the repository and clone it locally.
2. **Environment**: Ensure you have the required dependencies (GeoModel 6.22+, Geant4 11.x, SHiPGeometry, mp-units). Using the SHiP software stack via CVMFS is recommended.
3. **Pre-commit Hooks**: We use `pre-commit` to enforce coding standards. Install the hooks before making changes:
   ```bash
   pre-commit install
   ```
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
     cmake -S . -B build
     cmake --build build
     ctest --test-dir build --output-on-failure
     ```
   - Add tests for new features.
8. **Submission**: Open a Pull Request against the `main` branch. Ensure the CI passes.

## Coding Style

- **C++**: We use C++20. Style is defined in `.clang-format`.
- **CMake**: Formatting enforced by `gersemi`.

## Licensing

This project is licensed under the **LGPL-3.0-or-later**. All contributions must be compatible with this license. Each new file must include an SPDX identifier and copyright notice.
