# SHiP Geometry Service

A framework-agnostic C++23 library that wraps the SHiP GeoModel geometry behind a
stable interface. The design follows the patterns of DD4HEP's `IGeoSvc` (LHCb/key4hep)
and ALICE O2's `GeometryManager`.

## Documentation

An [automatic class reference](https://shipsoft.github.io/geometry_service/) is built using Doxygen from comments in the C++ code.

## Architecture

The library comes in three parts, so that a consumer pulls in only the
toolkits it uses:

| Target | Header | Dependencies in the interface |
|---|---|---|
| `SHiP::GeometryService` | `GeometryService.h` | none |
| `SHiP::GeometryServiceGeoModel` | `GeoModel.h` | GeoModelKernel |
| `SHiP::GeometryServiceG4` | `Geant4Geometry.h`, `G4RayScanner.h`, `GeometryThread.h` | Geant4 |

```
GeometryService                       core: loads and owns the GeoModel tree
  ::fromFile(db)                      reads a GeoModel .db file
  ::sharedFromFile(db)                one instance per file and process
resolveGeometryDbPath(name)           finds installed .db files

geoModelWorld(service)                GeoModel component
geometryServiceFromWorld(world)

Geant4Geometry::of(service)           G4 component: one conversion per service
  .worldLogical()                     lazy, call_once
  .logicalVolume(name)
G4RayScanner                          straight-line material scan
geometry_thread()                     the one thread allowed to create Geant4 geometry
clean_geant4_geometry_stores()
```

Design decisions:

- GeoModel stays out of the core header, so tracking and digitisation code
  can load and pass the geometry around without seeing GeoModel or Geant4.
- The GeoModel tree is built eagerly, in the factory functions, so geometry
  errors surface at startup.
- The GeoModel to Geant4 conversion is lazy (guarded by `std::call_once`).
  Users that only need the GeoModel tree never pay for it, and the caller
  decides which thread creates the Geant4 volumes. No `G4RunManager` is needed.
- Geant4 MT allows only one geometry-creating thread per process. When more
  than one component touches Geant4 geometry, run conversion and scanning
  through `SHiP::geometry_thread()`.

## Prerequisites

The easiest setup is [pixi](https://pixi.sh), which provisions the full
toolchain from `conda-forge` and the SHiP channel at `prefix.dev/ship-experiment/stable`.
For manual builds you'll need:

- CMake 3.24+
- C++23 compiler
- GeoModel 6.30+ (GeoModelCore, GeoModelIO; GeoModelG4 for the G4 component)
- Geant4 11.3+ for the G4 component (`-DBUILD_G4=OFF` skips it)

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

If you already have GeoModel and Geant4
installed (e.g. via spack or a system package manager), you can build
directly with CMake:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/path/to/GeoModel/install;/path/to/Geant4/install"
cmake --build build
cmake --install build --prefix /path/to/install
```

## API

### Loading geometry from a `.db` file

```cpp
#include <SHiP/GeometryService/GeometryService.h>

auto svc = SHiP::GeometryService::fromFile("/path/to/ship_geometry.db");
```

### Sharing one geometry file across plugins

```cpp
// Same instance for every caller that names the same file. A bare file name
// is looked up in $SHIPGEOMETRY_ROOT/share/geometry.
auto svc = SHiP::GeometryService::sharedFromFile(
    SHiP::resolveGeometryDbPath("ship_geometry.db"));
```

### Accessing the GeoModel tree

```cpp
#include <SHiP/GeometryService/GeoModel.h>

PVConstLink world = SHiP::geoModelWorld(*svc);

// Wrap a tree built in memory, e.g. by SHiP::geometry::DetectorBuilder
auto built = SHiP::geometryServiceFromWorld(SHiP::geometry::DetectorBuilder{}.build());
```

### Accessing Geant4 geometry

`Geant4Geometry::of()` returns the one Geant4 view of a service, shared by
every caller in the process. Keep it alive while you use its volumes. The
first call to `worldLogical()` converts the GeoModel tree. If anything else in
the process creates Geant4 geometry, convert on the shared geometry thread:

```cpp
#include <SHiP/GeometryService/Geant4Geometry.h>
#include <SHiP/GeometryService/GeometryThread.h>

auto g4 = SHiP::Geant4Geometry::of(svc);
auto& gt = SHiP::geometry_thread();
G4LogicalVolume& worldLV = gt.run([&]() -> auto& { return g4->worldLogical(); });
```

In a plain single-threaded Geant4 application, calling it from
`G4VUserDetectorConstruction::Construct()` also works:

```cpp
G4VPhysicalVolume* Construct() override {
    G4LogicalVolume& worldLV = g4_->worldLogical();
    return new G4PVPlacement(nullptr, G4ThreeVector(), &worldLV,
                             "World", nullptr, false, 0);
}
```

### Volume look-up

```cpp
// Valid after worldLogical() has been called
G4LogicalVolume* lv = g4->logicalVolume("MuonShield");  // nullptr if absent
```

### Material scans

`G4RayScanner` lists the materials a straight line crosses, with lengths in mm.
It needs no `G4RunManager`. Only one scanner may be alive at a time, because it
closes the Geant4 geometry, and that flag is process-wide.

```cpp
#include <SHiP/GeometryService/G4RayScanner.h>

SHiP::G4RayScanner scanner{&worldLV};
auto [entry, segments] = scanner.scan({0, 0, -50 * m}, {0, 0, 1});
for (const auto& [material, length] : segments)
    std::cout << material->GetName() << ' ' << length << " mm\n";
```

## Using from CMake

```cmake
# Core only: finds neither GeoModel nor Geant4
find_package(SHiPGeometryService 0.7 CONFIG REQUIRED)
target_link_libraries(mylib PRIVATE SHiP::GeometryService)

# With the optional components
find_package(SHiPGeometryService 0.7 CONFIG REQUIRED COMPONENTS GeoModel G4)
target_link_libraries(mysim PRIVATE SHiP::GeometryServiceG4 SHiP::GeometryServiceGeoModel)
```

The package config looks up GeoModelCore for the `GeoModel` and `G4`
components, and Geant4 for `G4`, but only when they are requested.

### Upgrading to 0.7

0.7.0 splits the library into the three targets above and drops the
dependency on `SHiPGeometry`. Request the components you use in
`find_package` and link their targets.

| 0.6 | 0.7 |
|---|---|
| `GeometryService::fromSource()` | `geometryServiceFromWorld(SHiP::geometry::DetectorBuilder{}.build())` |
| `GeometryService::fromWorld(world)` | `geometryServiceFromWorld(world)` (`GeoModel.h`) |
| `svc->geoModelWorld()` | `geoModelWorld(*svc)`, which returns a `PVConstLink` |
| `svc->geant4WorldLogical()` | `Geant4Geometry::of(svc)->worldLogical()` |
| `svc->getLogicalVolume(name)` | `Geant4Geometry::of(svc)->logicalVolume(name)` |
| `IGeometryService` | `GeometryService`, plus `Geant4Geometry` for Geant4 |
| `SHiP::GeometryService` linking Geant4 | `SHiP::GeometryServiceG4` |

In conda, `geometry-service-core` has the core and GeoModel components and
`geometry-service-g4` has the Geant4 one. The `shipgeometryservice`
metapackage installs both for one release cycle.

## Licence

LGPL-3.0-or-later. Copyright CERN for the benefit of the SHiP Collaboration.
