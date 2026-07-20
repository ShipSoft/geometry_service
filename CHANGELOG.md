# Changelog

All notable changes to this project will be documented in this file.

## [0.3.0] - 2026-07-20

### Features

- Add a process-wide geometry thread and shared service registry

### Bug Fixes

- Construct the geometry thread only via geometry_thread() and never tear it down
- Retain converted GeoModel trees for the process lifetime

### Testing

- Compare ownership instead of addresses in reload test
## [0.2.0] - 2026-07-16

### Features

- *(doxygen)* Add API documentation via Doxygen and GitHub Pages
- Enable consumption as a pixi source dependency
- Document opt-in dev environment for shipgeometry co-development
- Add G4RayScanner for straight-line material scans

### Bug Fixes

- Unroll install-hooks loop so deno_task_shell can parse it
- Build and run tests in CI
- Delete misleading defaulted move operations
- Reject missing files in fromFile

### Refactor

- [**breaking**] Remove magnetic field service
- Look up logical volumes via G4LogicalVolumeStore::GetVolume

### Documentation

- Link Doxygen API reference from README
- Document prek hooks in CONTRIBUTING

### Testing

- Migrate from GoogleTest to Catch2

### Build

- Add eigen-abi-devel dependency
- Modernize CMake dependency handling

### Miscellaneous

- Add pixi lock update workflow
- Lint with prek via pixi
- Adopt Renovate and scheduled pixi-lock updates
- Update pixi lock file
- Enforce conventional commits in CI via commit-check
- Add concurrency control and shared-config sync
## [0.1.0] - 2026-06-18

### Features

- Initial public release of SHiP Geometry Service
- Propagate dependency prefixes in installed CMake config
- *(build)* Default BUILD_SHARED_LIBS to ON

### Bug Fixes

- Preserve consumer's CMAKE_PREFIX_PATH in installed config
- Gate tests/ subdirectory on BUILD_TESTING
- Drop unused Microsoft.GSL dependency
- *(pre-commit)* Use upstream committed hook

### Documentation

- Document pixi as the recommended build path

### Styling

- Apply clang-format to all C++ sources
- Pre-commit fixes

### Miscellaneous

- Build and test with pixi instead of CVMFS
- Skip pixi.lock in codespell
- Use shared ShipSoft/.github reusable pixi-cmake-build workflow
- Add release automation via git-cliff
- *(release)* V0.1.0
