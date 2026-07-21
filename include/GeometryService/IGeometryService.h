// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <string>

class GeoVPhysVol;
class G4LogicalVolume;

namespace ship {

/// Framework-agnostic interface to the SHiP detector geometry.
/// Ownership of all returned pointers stays with the service.
class IGeometryService {
   public:
    virtual ~IGeometryService() = default;

    /// GeoModel world physical volume (const — tree is immutable after build).
    [[nodiscard]] virtual const GeoVPhysVol* geoModelWorld() const = 0;

    /// G4LogicalVolume* for the world. Triggers GeoModel2G4 on first call,
    /// which must run on the process-wide geometry thread
    /// (ship::geometry_thread(), see GeometryThread.h) whenever more than
    /// one thread touches Geant4 geometry.
    [[nodiscard]] virtual G4LogicalVolume* geant4WorldLogical() = 0;

    /// Look up a G4LogicalVolume by name within this service's converted
    /// tree (depth-first). Returns nullptr before geant4WorldLogical() has
    /// been called or when no such volume exists.
    [[nodiscard]] virtual G4LogicalVolume* getLogicalVolume(const std::string& name) const = 0;
};

}  // namespace ship
