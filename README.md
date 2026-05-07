# SHiP Geometry Service

A framework-agnostic C++20 library that wraps the SHiP GeoModel geometry behind a
stable interface. The design follows the patterns of DD4HEP's `IGeoSvc` (LHCb/key4hep)
and ALICE O2's `GeometryManager`.

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
│  SHiPGeometryService                    │
│  ::fromSource()   — SHiPGeometryBuilder │
│  ::fromFile(db)   — GeoModelIO::Read    │
└─────────────────────────────────────────┘
```

**Design decisions:**

- `IGeometryService` has no framework headers — can be used standalone or from any
  framework.
- GeoModel tree construction is **eager** (in the factory functions) to surface
  geometry errors at startup.
- GeoModel→Geant4 conversion (`geant4WorldLogical()`) is **lazy** (guarded by
  `std::call_once`) because `ExtParameterisedVolumeBuilder::Build()` requires a
  `G4RunManager` to exist on the calling thread. Call it from within
  `G4VUserDetectorConstruction::Construct()`.

## Prerequisites

- CMake 3.20+
- C++20 compiler
- `SHiPGeometry` installed (from `geometry/` repository)
- GeoModel 6.22+ (GeoModelCore, GeoModelIO, GeoModelG4)
- Geant4 11.x

On CVMFS-enabled systems:

```bash
source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh
export LD_LIBRARY_PATH=/path/to/GeoModel/install/lib64:$LD_LIBRARY_PATH
```

## Building

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
#include <GeometryService/SHiPGeometryService.h>

auto svc = ship::SHiPGeometryService::fromSource();
const GeoVPhysVol* world = svc->geoModelWorld();
```

### Loading geometry from a `.db` file

```cpp
auto svc = ship::SHiPGeometryService::fromFile("/path/to/ship_geometry.db");
```

### Accessing Geant4 geometry

Call `geant4WorldLogical()` from within `G4VUserDetectorConstruction::Construct()`:

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

## Using from CMake

```cmake
find_package(SHiPGeometryService CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE SHiPGeometryService::SHiPGeometryService)
```

The package config pulls in `SHiPGeometry`, `GeoModelCore`, `GeoModelIO`,
`GeoModelG4`, and `Geant4` automatically.

## Licence

LGPL-3.0-or-later. Copyright CERN for the benefit of the SHiP Collaboration.
