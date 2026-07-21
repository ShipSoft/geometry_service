// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "GeometryService/IGeometryService.h"

#include <GeoModelKernel/GeoVPhysVol.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

class G4LogicalVolume;

namespace ship {

/// Concrete implementation of IGeometryService backed by GeoModel.
///
/// Four factory functions:
///   SHiPGeometryService::fromSource()          — builds via SHiPGeometryBuilder
///   SHiPGeometryService::fromFile(dbPath)      — reads from a GeoModel .db file
///   SHiPGeometryService::sharedFromFile(dbPath)— process-wide shared instance
///   SHiPGeometryService::fromWorld(world)      — wraps an existing GeoModel tree
class SHiPGeometryService final : public IGeometryService {
   public:
    /// Build geometry from the C++ SHiPGeometryBuilder.
    [[nodiscard]] static std::unique_ptr<SHiPGeometryService> fromSource();

    /// Load geometry from a previously written GeoModel SQLite database.
    [[nodiscard]] static std::unique_ptr<SHiPGeometryService> fromFile(
        const std::filesystem::path& dbPath);

    /// Process-wide shared instance per geometry file, keyed by the
    /// canonical path. The first caller loads the file; later callers —
    /// from any plugin linking this library — get the same instance, so
    /// the GeoModel tree is loaded and converted at most once per process
    /// and freed once the last owner releases it.
    ///
    /// All conversion and navigation on a shared instance must run on
    /// ship::geometry_thread() (see GeometryThread.h): Geant4 permits only
    /// one geometry-creating thread per process. Converted GeoModel trees
    /// are retained for the process lifetime (see geant4WorldLogical), so
    /// reloading a file after a converted instance has expired is safe.
    [[nodiscard]] static std::shared_ptr<SHiPGeometryService> sharedFromFile(
        const std::filesystem::path& dbPath);

    /// Wrap an already-built GeoModel world (e.g. a small test geometry).
    [[nodiscard]] static std::unique_ptr<SHiPGeometryService> fromWorld(PVConstLink world);

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
    std::exception_ptr m_conversionError;   ///< sticky conversion failure
};

}  // namespace ship
