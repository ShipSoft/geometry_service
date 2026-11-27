// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <filesystem>
#include <memory>

namespace SHiP {

/// The SHiP detector geometry, loaded once and immutable afterwards.
///
/// This header exposes neither GeoModel nor Geant4. Optional components give
/// access to the geometry in a given toolkit:
///   - SHiP::GeometryServiceGeoModel (SHiP/GeometryService/GeoModel.h): the
///     GeoModel world, and services wrapping an existing GeoModel tree.
///   - SHiP::GeometryServiceG4 (SHiP/GeometryService/Geant4Geometry.h): the
///     Geant4 conversion, ray scanning and the geometry thread.
class GeometryService final {
   public:
    /// Load geometry from a previously written GeoModel SQLite database.
    [[nodiscard]] static std::unique_ptr<GeometryService> fromFile(
        const std::filesystem::path& dbPath);

    /// Process-wide shared instance per geometry file, keyed by the
    /// canonical path. The first caller loads the file; later callers, from
    /// any plugin linking this library, get the same instance, so the
    /// geometry is loaded at most once per process and freed once the last
    /// owner releases it.
    [[nodiscard]] static std::shared_ptr<GeometryService> sharedFromFile(
        const std::filesystem::path& dbPath);

    ~GeometryService();

    // Non-copyable and non-movable: instances are only handed out via
    // unique_ptr or shared_ptr, and components key their state on the address.
    GeometryService(const GeometryService&) = delete;
    GeometryService& operator=(const GeometryService&) = delete;
    GeometryService(GeometryService&&) = delete;
    GeometryService& operator=(GeometryService&&) = delete;

   private:
    struct Impl;                          ///< the GeoModel tree; defined in the library only
    friend struct GeometryServiceAccess;  ///< library-internal, see src/

    explicit GeometryService(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> m_impl;
};

/// Path of a GeoModel geometry database. A path that exists is returned as
/// given. A bare file name that does not exist is looked up in
/// `$SHIPGEOMETRY_ROOT/share/geometry/`, the flat install layout of the
/// shipgeometry package. Throws `std::runtime_error` otherwise. That includes
/// a missing path with directory components: falling back to the installed
/// geometry for a mistyped path would run the job on the wrong detector.
[[nodiscard]] std::filesystem::path resolveGeometryDbPath(const std::filesystem::path& path);

}  // namespace SHiP
