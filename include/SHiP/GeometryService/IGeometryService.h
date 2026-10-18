// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <string_view>

class GeoVPhysVol;  // NOLINT(cppcoreguidelines-virtual-class-destructor): GeoModel's class
class G4LogicalVolume;

namespace SHiP {

/// Framework-agnostic interface to the SHiP detector geometry.
/// Ownership of all returned volumes stays with the service.
class IGeometryService {
   public:
    virtual ~IGeometryService() = default;

    /// GeoModel world physical volume (const — tree is immutable after build).
    [[nodiscard]] virtual const GeoVPhysVol* geoModelWorld() const = 0;

    /// The Geant4 world logical volume. Triggers GeoModel2G4 on first call,
    /// which must run on the process-wide geometry thread
    /// (SHiP::geometry_thread(), see GeometryThread.h) whenever more than
    /// one thread touches Geant4 geometry. Throws if the conversion fails,
    /// on this and every later call, so it never yields a null volume.
    /// When returning it from GeometryThread::run, give the lambda a
    /// reference return type (`[&]() -> auto& { ... }`).
    [[nodiscard]] virtual G4LogicalVolume& geant4WorldLogical() = 0;

    /// Look up a G4LogicalVolume by name within this service's converted
    /// tree (depth-first). Returns nullptr before geant4WorldLogical() has
    /// been called or when no such volume exists; rethrows a stored
    /// conversion failure instead of masking it as nullptr.
    [[nodiscard]] virtual G4LogicalVolume* getLogicalVolume(std::string_view name) const = 0;
};

}  // namespace SHiP
