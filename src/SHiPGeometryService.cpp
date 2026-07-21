// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>

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
#include <cstddef>
#include <exception>
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

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromFile(
    const std::filesystem::path& dbPath) {
    // GMDBManager opens via sqlite3_open, which silently creates a missing
    // file; validate up front to fail with a clear message instead.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("SHiPGeometryService::fromFile: no such file: " + dbPath.string());
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    auto db = std::make_shared<GMDBManager>(dbPath.string());
    // An existing but corrupt or non-GeoModel file leaves the manager
    // unopened; ReadGeoModel would fail deep inside library code otherwise.
    if (!db->checkIsDBOpen())
        throw std::runtime_error("SHiPGeometryService::fromFile: not a GeoModel database: " +
                                 dbPath.string());
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
    const std::filesystem::path& dbPath) {
    static std::mutex registryMutex;
    static std::map<std::string, std::weak_ptr<SHiPGeometryService>> registry;

    // Canonicalisation needs an existing file; fromFile would also reject a
    // missing one, but only after the registry lookup keyed a bogus path.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("SHiPGeometryService::sharedFromFile: no such file: " +
                                 dbPath.string());
    const auto key = std::filesystem::canonical(dbPath).string();

    // The lock is held across the load so concurrent first callers cannot
    // both read the file; contention is limited to job initialisation.
    std::lock_guard lk{registryMutex};
    std::erase_if(registry, [](const auto& entry) { return entry.second.expired(); });
    auto& slot = registry[key];
    if (auto existing = slot.lock())
        return existing;
    std::shared_ptr<SHiPGeometryService> svc = fromFile(key);
    slot = svc;
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
        // Pre-MR-612 GeoModel2G4 caches conversions in static maps keyed by
        // GeoModel node pointers, so a tree handed to the builder must never
        // be freed: recycled node addresses would turn the cache into
        // dangling lookups. Retain every such tree for the process lifetime
        // (leaked so the nodes also survive static teardown) — before
        // converting, so nodes cached by a partial, failed conversion stay
        // alive too. With MR 612's per-instance caches this is unnecessary.
        {
            static std::mutex retainedMutex;
            static auto* retained = new std::vector<PVConstLink>;
            std::lock_guard lk{retainedMutex};
            retained->push_back(m_world);
        }
#endif
        // A failed conversion is sticky: it can leave partially registered
        // volumes in the Geant4 stores (and, pre-MR-612, half-populated
        // static caches), so a retry could silently produce a mixed tree.
        // Store the error and rethrow it on every call instead.
        try {
#if SHIP_GEOMODEL2G4_STATIC_CACHES
            ExtParameterisedVolumeBuilder builder("SHiP");
#else
            VolumeBuilder builder{"SHiP"};
#endif
            m_g4WorldLV = builder.Build(m_world);
            if (!m_g4WorldLV)
                throw std::runtime_error("SHiPGeometryService: GeoModel→Geant4 conversion failed");
        } catch (...) {
            m_conversionError = std::current_exception();
        }
    });
    if (m_conversionError)
        std::rethrow_exception(m_conversionError);
    return m_g4WorldLV;
}

namespace {

G4LogicalVolume* findLogicalVolume(G4LogicalVolume* lv, const std::string& name) {
    if (lv->GetName() == name)
        return lv;
    for (std::size_t i = 0; i < lv->GetNoDaughters(); ++i)
        if (auto* found = findLogicalVolume(lv->GetDaughter(i)->GetLogicalVolume(), name))
            return found;
    return nullptr;
}

}  // namespace

G4LogicalVolume* SHiPGeometryService::getLogicalVolume(const std::string& name) const {
    // Search this service's own tree rather than G4LogicalVolumeStore:
    // retained trees of expired instances of the same file leave
    // identically named volumes in the store, and a store lookup would
    // return the first — stale — match.
    if (!m_g4WorldLV)
        return nullptr;
    return findLogicalVolume(m_g4WorldLV, name);
}

}  // namespace ship
