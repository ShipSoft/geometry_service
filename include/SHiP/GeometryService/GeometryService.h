// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SHiP/GeometryService/IGeometryService.h"

#include <GeoModelKernel/GeoVPhysVol.h>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class G4LogicalVolume;

namespace SHiP {

/// Concrete implementation of IGeometryService backed by GeoModel.
///
/// Four factory functions:
///   GeometryService::fromSource()          — builds via SHiP::geometry::DetectorBuilder
///   GeometryService::fromFile(dbPath)      — reads from a GeoModel .db file
///   GeometryService::sharedFromFile(dbPath)— process-wide shared instance
///   GeometryService::fromWorld(world)      — wraps an existing GeoModel tree
class GeometryService final : public IGeometryService {
   public:
    /// Build geometry from the C++ SHiP::geometry::DetectorBuilder.
    [[nodiscard]] static std::unique_ptr<GeometryService> fromSource();

    /// Load geometry from a previously written GeoModel SQLite database.
    [[nodiscard]] static std::unique_ptr<GeometryService> fromFile(
        const std::filesystem::path& dbPath);

    /// Process-wide shared instance per geometry file, keyed by the
    /// canonical path. The first caller loads the file; later callers —
    /// from any plugin linking this library — get the same instance, so
    /// the GeoModel tree is loaded and converted at most once per process
    /// and freed once the last owner releases it.
    ///
    /// All conversion and navigation on a shared instance must run on
    /// SHiP::geometry_thread() (see GeometryThread.h): Geant4 permits only
    /// one geometry-creating thread per process. Reloading a file after a
    /// converted instance has expired is safe: each conversion uses its own
    /// VolumeBuilder caches, while the Geant4 volumes of the expired
    /// conversion stay in the global stores.
    [[nodiscard]] static std::shared_ptr<GeometryService> sharedFromFile(
        const std::filesystem::path& dbPath);

    /// Wrap an already-built GeoModel world (e.g. a small test geometry).
    [[nodiscard]] static std::unique_ptr<GeometryService> fromWorld(PVConstLink world);

    ~GeometryService() override = default;

    // Non-copyable and non-movable (std::once_flag member; instances are
    // only handed out via unique_ptr or shared_ptr, so moves are not needed).
    GeometryService(const GeometryService&) = delete;
    GeometryService& operator=(const GeometryService&) = delete;
    GeometryService(GeometryService&&) = delete;
    GeometryService& operator=(GeometryService&&) = delete;

    const GeoVPhysVol* geoModelWorld() const override;
    G4LogicalVolume& geant4WorldLogical() override;
    G4LogicalVolume* getLogicalVolume(std::string_view name) const override;

   private:
    GeometryService() = default;

    /// Transparent, so getLogicalVolume() looks a string_view up without
    /// building a std::string.
    struct NameHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view name) const noexcept {
            return std::hash<std::string_view>{}(name);
        }
    };

    PVConstLink m_world;  ///< intrusive ref-counted GeoModel tree
    std::once_flag m_g4ConversionDone;
    G4LogicalVolume* m_g4WorldLV{nullptr};  ///< written once via call_once
    std::exception_ptr m_conversionError;   ///< sticky conversion failure
    /// First converted volume of each name, depth-first; filled with
    /// m_g4WorldLV, so later renames are not seen.
    std::unordered_map<std::string, G4LogicalVolume*, NameHash, std::equal_to<>> m_lvByName;
};

}  // namespace SHiP
