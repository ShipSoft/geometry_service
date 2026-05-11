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

    /// G4LogicalVolume* for the world. Triggers GeoModel2G4 on first call.
    /// Must be called from within G4VUserDetectorConstruction::Construct().
    [[nodiscard]] virtual G4LogicalVolume* geant4WorldLogical() = 0;

    /// Look up a G4LogicalVolume by name (via G4LogicalVolumeStore).
    /// Valid only after geant4WorldLogical() has been called.
    [[nodiscard]] virtual G4LogicalVolume* getLogicalVolume(const std::string& name) const = 0;
};

}  // namespace ship
