// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/GeometryService.h"

#include "SHiP/GeometryService/GeoModel.h"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelRead/ReadGeoModel.h>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace SHiP {

struct GeometryService::Impl {
    PVConstLink world;  ///< intrusive ref-counted GeoModel tree
};

/// Grants this file's free functions access to GeometryService internals.
struct GeometryServiceAccess {
    static std::unique_ptr<GeometryService> make(PVConstLink world) {
        auto impl =
            std::make_unique<GeometryService::Impl>(GeometryService::Impl{std::move(world)});
        return std::unique_ptr<GeometryService>(new GeometryService(std::move(impl)));
    }
    static const PVConstLink& world(const GeometryService& service) noexcept {
        return service.m_impl->world;
    }
};

GeometryService::GeometryService(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}

GeometryService::~GeometryService() = default;

// ----------------------------------------------------------------------------
// Factory: load from GeoModel .db file
// ----------------------------------------------------------------------------

std::unique_ptr<GeometryService> GeometryService::fromFile(const std::filesystem::path& dbPath) {
    // GMDBManager opens via sqlite3_open, which silently creates a missing
    // file; validate up front to fail with a clear message instead.
    if (!std::filesystem::is_regular_file(dbPath)) {
        throw std::runtime_error("GeometryService::fromFile: no such file: " + dbPath.string());
    }
    const auto db = std::make_shared<GMDBManager>(dbPath.string());
    // An existing but corrupt or non-GeoModel file leaves the manager
    // unopened; ReadGeoModel would fail deep inside library code otherwise.
    if (!db->checkIsDBOpen()) {
        throw std::runtime_error("GeometryService::fromFile: not a GeoModel database: " +
                                 dbPath.string());
    }
    GeoModelIO::ReadGeoModel reader(db);
    PVConstLink world = reader.buildGeoModel();
    if (!world) {
        throw std::runtime_error("GeometryService::fromFile: buildGeoModel returned null");
    }
    return GeometryServiceAccess::make(std::move(world));
}

// ----------------------------------------------------------------------------
// Factory: process-wide shared instance per geometry file
// ----------------------------------------------------------------------------

std::shared_ptr<GeometryService> GeometryService::sharedFromFile(
    const std::filesystem::path& dbPath) {
    static std::mutex registryMutex;
    static std::map<std::string, std::weak_ptr<GeometryService>> registry;

    // Canonicalisation needs an existing file; fromFile would also reject a
    // missing one, but only after the registry lookup keyed a bogus path.
    if (!std::filesystem::is_regular_file(dbPath)) {
        throw std::runtime_error("GeometryService::sharedFromFile: no such file: " +
                                 dbPath.string());
    }
    const auto key = std::filesystem::canonical(dbPath).string();

    // The lock is held across the load so concurrent first callers cannot
    // both read the file; contention is limited to job initialisation.
    const std::scoped_lock lk{registryMutex};
    std::erase_if(registry, [](const auto& entry) { return entry.second.expired(); });
    auto& slot = registry[key];
    if (auto existing = slot.lock()) {
        return existing;
    }
    std::shared_ptr<GeometryService> svc = fromFile(key);
    slot = svc;
    return svc;
}

// ----------------------------------------------------------------------------
// GeoModel interop (SHiP/GeometryService/GeoModel.h)
// ----------------------------------------------------------------------------

std::unique_ptr<GeometryService> geometryServiceFromWorld(PVConstLink world) {
    if (!world) {
        throw std::runtime_error("geometryServiceFromWorld: world is null");
    }
    return GeometryServiceAccess::make(std::move(world));
}

PVConstLink geoModelWorld(const GeometryService& service) {
    return GeometryServiceAccess::world(service);
}

// ----------------------------------------------------------------------------
// Geometry database lookup
// ----------------------------------------------------------------------------

std::filesystem::path resolveGeometryDbPath(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        return path;
    }
    if (!path.empty() && path == path.filename()) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe): read-only, at configuration time
        if (const auto* root = std::getenv("SHIPGEOMETRY_ROOT")) {
            auto resolved = std::filesystem::path{root} / "share" / "geometry" / path;
            if (std::filesystem::exists(resolved)) {
                return resolved;
            }
        }
    }
    throw std::runtime_error("Cannot locate geometry database '" + path.string() +
                             "'; set SHIPGEOMETRY_ROOT or provide an absolute path");
}

}  // namespace SHiP
