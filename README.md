# SHiP Geometry Service

A framework-agnostic C++23 library that wraps the SHiP GeoModel geometry behind a
stable interface. The design follows the patterns of DD4HEP's `IGeoSvc` (LHCb/key4hep)
and ALICE O2's `GeometryManager`.

## Documentation

An [automatic class reference](https://shipsoft.github.io/geometry_service/) is built using Doxygen from comments in the C++ code.

## Architecture

```
┌─────────────────────────────────────────┐
│  IGeometryService  (abstract interface) │
│  • geoModelWorld()  → GeoVPhysVol*      │
│  • geant4WorldLogical() → G4LogicalVol* │  ← lazy, call_once
│  • getLogicalVolume(name)               │
└───────────────────┬─────────────────────┘
                    │ implements
┌───────────────────▼─────────────────────┐
│  GeometryService                        │
│  ::fromSource()       — C++ builder     │
│  ::fromFile(db)       — GeoModelIO::Read│
│  ::sharedFromFile(db) — one per file    │
│  ::fromWorld(world)   — existing tree   │
└─────────────────────────────────────────┘

G4RayScanner     straight-line material scan over a converted tree
geometry_thread  the one thread allowed to create Geant4 geometry
```

**Design decisions:**

- `IGeometryService` has no framework headers, so it can be used standalone or
  from any framework.
- GeoModel tree construction is **eager** (in the factory functions) to surface
  geometry errors at startup.
- GeoModel→Geant4 conversion (`geant4WorldLogical()`) is **lazy** (guarded by
  `std::call_once`). Users that only need the GeoModel tree never pay for it,
  and the caller decides which thread creates the Geant4 volumes. No
  `G4RunManager` is needed.
- Geant4 MT allows only one geometry-creating thread per process. When more
  than one component touches Geant4 geometry, run conversion and scanning
  through `SHiP::geometry_thread()`.

## Prerequisites

The easiest setup is [pixi](https://pixi.sh), which provisions the full
toolchain from `conda-forge` and the SHiP channel at `prefix.dev/ship-experiment/stable`.
For manual builds you'll need:

- CMake 3.24+
- C++23 compiler
- `SHiPGeometry` installed (from `geometry/` repository)
- GeoModel 6.30+ (GeoModelCore, GeoModelIO, GeoModelG4)
- Geant4 11.x

## Building

### With pixi (recommended)

```bash
pixi run test
```

That single command provisions the env, configures with CMake/Ninja
(`RelWithDebInfo`), builds, and runs ctest — the same chain CI uses.

For iteration, the individual tasks defined in `pixi.toml` are:

```bash
pixi run configure   # cmake -S . -B build -G Ninja ...
pixi run build       # cmake --build build -j
pixi run test        # ctest --test-dir build --output-on-failure
```

Use `pixi shell` to drop into an interactive shell with the env activated
(useful for running individual binaries, debuggers, etc.).

### Manual build

If you already have GeoModel, Geant4, and `SHiPGeometry`
installed (e.g. via spack or a system package manager), you can build
directly with CMake:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/path/to/GeoModel/install;/path/to/SHiPGeometry/install"
cmake --build build
cmake --install build --prefix /path/to/install
```

## API

### Building geometry from source

```cpp
#include <SHiP/GeometryService/GeometryService.h>

auto svc = SHiP::GeometryService::fromSource();
const GeoVPhysVol* world = svc->geoModelWorld();
```

### Loading geometry from a `.db` file

```cpp
auto svc = SHiP::GeometryService::fromFile("/path/to/ship_geometry.db");
```

### Sharing one geometry file across plugins

```cpp
// Same instance for every caller that names the same file
auto svc = SHiP::GeometryService::sharedFromFile("/path/to/ship_geometry.db");
```

### Accessing Geant4 geometry

The first call to `geant4WorldLogical()` converts the GeoModel tree. If
anything else in the process creates Geant4 geometry, convert on the shared
geometry thread:

```cpp
#include <SHiP/GeometryService/GeometryThread.h>

auto& gt = SHiP::geometry_thread();
G4LogicalVolume* worldLV = gt.run([&] { return svc->geant4WorldLogical(); });
```

In a plain single-threaded Geant4 application, calling it from
`G4VUserDetectorConstruction::Construct()` also works:

```cpp
G4VPhysicalVolume* Construct() override {
    G4LogicalVolume* worldLV = geoSvc_.geant4WorldLogical();
    return new G4PVPlacement(nullptr, G4ThreeVector(), worldLV,
                             "World", nullptr, false, 0);
}
```

### Volume look-up

```cpp
// Valid after geant4WorldLogical() has been called
G4LogicalVolume* lv = svc->getLogicalVolume("MuonShield");
```

### Material scans

`G4RayScanner` lists the materials a straight line crosses, with lengths in mm.
It needs no `G4RunManager`. Only one scanner may be alive at a time, because it
closes the Geant4 geometry, and that flag is process-wide.

```cpp
#include <SHiP/GeometryService/G4RayScanner.h>

SHiP::G4RayScanner scanner{worldLV};
auto [entry, segments] = scanner.scan({0, 0, -50 * m}, {0, 0, 1});
for (const auto& [material, length] : segments)
    std::cout << material->GetName() << ' ' << length << " mm\n";
```

## Using from CMake

```cmake
find_package(SHiPGeometryService CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE SHiP::GeometryService)
```

The package config pulls in `SHiPGeometry`, `GeoModelCore`, `GeoModelIO`,
`GeoModelG4`, and `Geant4` automatically.

### Upgrading from 0.4

0.5.0 moved everything from `ship` into `SHiP` and dropped the stutter from
the names:

| Before | After |
|---|---|
| `#include <GeometryService/SHiPGeometryService.h>` | `#include <SHiP/GeometryService/GeometryService.h>` |
| `#include <GeometryService/X.h>` | `#include <SHiP/GeometryService/X.h>` |
| `ship::SHiPGeometryService` | `SHiP::GeometryService` |
| `ship::X` | `SHiP::X` |
| `SHiPGeometryService::SHiPGeometryService` | `SHiP::GeometryService` |

The old header paths, the old class name, a `namespace ship = SHiP;` alias and
the old CMake target keep working until 0.6.0. The alias comes from the old
header paths only, and it conflicts with any other declaration of `namespace ship`
in the same translation unit. That covers data-model releases before 0.6.0 as
well as your own code, including forward declarations of the old types such as
`namespace ship { class SHiPGeometryService; }`. Code that declares anything in
`namespace ship` must include the new header paths. Define
`SHIP_NO_DEPRECATED_NAMESPACE_ALIAS` to drop the alias early.

## Licence

LGPL-3.0-or-later. Copyright CERN for the benefit of the SHiP Collaboration.
