// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "GeometryService/IGeometryService.h"

#include <GeoModelKernel/GeoVPhysVol.h>
#include <memory>
#include <mutex>
#include <string>

class G4LogicalVolume;

namespace ship {

/// Concrete implementation of IGeometryService backed by GeoModel.
///
/// Two factory functions:
///   SHiPGeometryService::fromSource()         — builds via SHiPGeometryBuilder
///   SHiPGeometryService::fromFile(dbPath)     — reads from a GeoModel .db file
class SHiPGeometryService final : public IGeometryService {
   public:
    /// Build geometry from the C++ SHiPGeometryBuilder.
    static std::unique_ptr<SHiPGeometryService> fromSource();

    /// Load geometry from a previously written GeoModel SQLite database.
    static std::unique_ptr<SHiPGeometryService> fromFile(const std::string& dbPath);

    ~SHiPGeometryService() override = default;

    // Non-copyable and non-movable (std::once_flag member; instances are
    // only handed out via unique_ptr, so moves are not needed).
    SHiPGeometryService(const SHiPGeometryService&) = delete;
    SHiPGeometryService& operator=(const SHiPGeometryService&) = delete;
    SHiPGeometryService(SHiPGeometryService&&) = delete;
    SHiPGeometryService& operator=(SHiPGeometryService&&) = delete;

    const GeoVPhysVol* geoModelWorld() const override;
    G4LogicalVolume* geant4WorldLogical() override;
    G4LogicalVolume* getLogicalVolume(const std::string& name) const override;

   private:
    SHiPGeometryService() = default;

    PVConstLink m_world;  ///< intrusive ref-counted GeoModel tree
    mutable std::once_flag m_g4ConversionDone;
    G4LogicalVolume* m_g4WorldLV{nullptr};  ///< written once via call_once
};

}  // namespace ship
