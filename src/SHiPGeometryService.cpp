// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <G4LogicalVolumeStore.hh>

// GeoModel MR 612 (GeoModelDev/GeoModel!612, our work item #135) merges
// ExtParameterisedVolumeBuilder into a concrete VolumeBuilder whose
// conversion caches are per-instance; support both APIs until the fix is
// the only one in the wild. The header probe can be overridden (=0) to
// force the new API when both header sets are visible (e.g. testing
// against a locally built GeoModel next to the installed one).
#ifndef SHIP_GEOMODEL2G4_STATIC_CACHES
#if __has_include(<GeoModel2G4/ExtParameterisedVolumeBuilder.h>)
#define SHIP_GEOMODEL2G4_STATIC_CACHES 1
#else
#define SHIP_GEOMODEL2G4_STATIC_CACHES 0
#endif
#endif
#if SHIP_GEOMODEL2G4_STATIC_CACHES
#include <GeoModel2G4/ExtParameterisedVolumeBuilder.h>
#else
#include <GeoModel2G4/VolumeBuilder.h>
#endif
#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelRead/ReadGeoModel.h>
#include <SHiPGeometry/SHiPGeometry.h>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace ship {

// ----------------------------------------------------------------------------
// Factory: build from C++ source
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromSource() {
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    SHiPGeometry::SHiPGeometryBuilder builder;
    svc->m_world = builder.build();
    if (!svc->m_world)
        throw std::runtime_error("SHiPGeometryService::fromSource: geometry build returned null");
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: load from GeoModel .db file
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromFile(const std::string& dbPath) {
    // GMDBManager opens via sqlite3_open, which silently creates a missing
    // file; validate up front to fail with a clear message instead.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("SHiPGeometryService::fromFile: no such file: " + dbPath);
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    auto db = std::make_shared<GMDBManager>(dbPath);
    GeoModelIO::ReadGeoModel reader(db);
    svc->m_world = reader.buildGeoModel();
    if (!svc->m_world)
        throw std::runtime_error("SHiPGeometryService::fromFile: buildGeoModel returned null");
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: process-wide shared instance per geometry file
// ----------------------------------------------------------------------------

std::shared_ptr<SHiPGeometryService> SHiPGeometryService::sharedFromFile(
    const std::string& dbPath) {
    static std::mutex registryMutex;
    static std::map<std::string, std::weak_ptr<SHiPGeometryService>> registry;

    // Canonicalisation needs an existing file; fromFile would also reject a
    // missing one, but only after the registry lookup keyed a bogus path.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("SHiPGeometryService::sharedFromFile: no such file: " + dbPath);
    const auto key = std::filesystem::canonical(dbPath).string();

    // The lock is held across the load so concurrent first callers cannot
    // both read the file; contention is limited to job initialisation.
    std::lock_guard lk{registryMutex};
    if (auto existing = registry[key].lock())
        return existing;
    std::shared_ptr<SHiPGeometryService> svc = fromFile(key);
    registry[key] = svc;
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: wrap an existing GeoModel tree
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromWorld(PVConstLink world) {
    if (!world)
        throw std::runtime_error("SHiPGeometryService::fromWorld: world is null");
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    svc->m_world = std::move(world);
    return svc;
}

// ----------------------------------------------------------------------------
// IGeometryService implementation
// ----------------------------------------------------------------------------

const GeoVPhysVol* SHiPGeometryService::geoModelWorld() const {
    return m_world.get();
}

G4LogicalVolume* SHiPGeometryService::geant4WorldLogical() {
    std::call_once(m_g4ConversionDone, [this]() {
#if SHIP_GEOMODEL2G4_STATIC_CACHES
        ExtParameterisedVolumeBuilder builder("SHiP");
#else
        VolumeBuilder builder{"SHiP"};
#endif
        m_g4WorldLV = builder.Build(m_world);
        if (!m_g4WorldLV)
            throw std::runtime_error("SHiPGeometryService: GeoModel→Geant4 conversion failed");
#if SHIP_GEOMODEL2G4_STATIC_CACHES
        // Pre-MR-612 GeoModel2G4 caches conversions in static maps keyed by
        // GeoModel node pointers, so a converted tree must never be freed:
        // recycled node addresses would turn the cache into dangling
        // lookups. Retain every converted tree for the process lifetime
        // (leaked so the nodes also survive static teardown). With MR 612's
        // per-instance caches this is unnecessary.
        static std::mutex retainedMutex;
        static auto* retained = new std::vector<PVConstLink>;
        std::lock_guard lk{retainedMutex};
        retained->push_back(m_world);
#endif
    });
    return m_g4WorldLV;
}

G4LogicalVolume* SHiPGeometryService::getLogicalVolume(const std::string& name) const {
    return G4LogicalVolumeStore::GetInstance()->GetVolume(name, /*verbose=*/false);
}

}  // namespace ship
