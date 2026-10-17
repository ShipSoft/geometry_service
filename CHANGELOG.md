# Changelog

All notable changes to this project will be documented in this file.

## [0.5.0] - 2026-10-17

### Bug fixes

- *(release)* Roll back on any failure, fail on unbumpable versions
- *(pixi)* Add expat, zlib and freetype for Geant4's CMake config
- Let a failed git diff fail the clang-tidy-diff task
- *(release)* Tag the release only after it lands on main
- *(scanner)* Reject zero or non-finite ray directions
- *(scanner)* Don't report a clean exit as a step-limit overrun
- *(scanner)* Don't leak the placement if construction fails
- Accept directions whose squared norm over- or underflows

### Refactor

- Install headers through a file set and drop a needless mutable
- [**breaking**] Move the geometry service into the SHiP namespace

### Documentation

- Adopt AI policy
- Describe what the clang-tidy gate actually does, and fix the local check
- Drop stale tree-retention claims and refresh the README
- Say the ship alias clashes with any namespace ship declaration

### Performance

- Index converted logical volumes by name

### Miscellaneous

- Sync shared configs
- Update pixi lock file
- Remove redundant PR checklist template
- Grant config-sync job explicit permissions
- Update pixi lock file
- Gate merges on uniform aggregator checks
- Check C++ against the Core Guidelines with clang-tidy
- Sync shared configs
- Require GeoModel 6.30 and drop the dual-API builder shim
- Add advisory builds against ShipSoft library mains

### Build

- Pass CMAKE_PREFIX_PATH to the configure task
- Get Catch2 from conda-forge instead of FetchContent
- Require GeoModel 6.30 in find_package and set SOVERSION
- Resolve SHiP packages from ship-experiment/stable

## [0.4.0] - 2026-08-10

### Features

- Support the post-MR-612 GeoModel2G4 VolumeBuilder API

### Bug Fixes

- Fail loud when the ray scanner cannot voxelise or terminate
- Harden geometry loading and scope volume lookup to the service
- Address review feedback on PR #32

### Refactor

- Modernise GeometryThread with jthread and stop_token

### Build

- Export the C++ standard and hint dependency prefixes
## [0.3.0] - 2026-07-20

### Features

- Add a process-wide geometry thread and shared service registry

### Bug Fixes

- Construct the geometry thread only via geometry_thread() and never tear it down
- Retain converted GeoModel trees for the process lifetime

### Testing

- Compare ownership instead of addresses in reload test

### Miscellaneous

- *(release)* V0.3.0
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
- *(release)* V0.2.0
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
