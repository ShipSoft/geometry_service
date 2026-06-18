# Changelog

All notable changes to this project will be documented in this file.

## [0.1.0] - 2026-06-18

### Features

- Initial public release of SHiP Geometry Service
- Propagate dependency prefixes in installed CMake config
- *(build)* Default BUILD_SHARED_LIBS to ON

### Bug Fixes

- Preserve consumer's CMAKE_PREFIX_PATH in installed config
- Gate tests/ subdirectory on BUILD_TESTING
- Drop unused Microsoft.GSL dependency

### Styling

- Apply clang-format to all C++ sources
- Pre-commit fixes

### Miscellaneous

- Add release automation via git-cliff
