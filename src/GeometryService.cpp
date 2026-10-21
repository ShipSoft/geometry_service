// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/GeometryService.h"

#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>

#include <GeoModel2G4/VolumeBuilder.h>
#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelRead/ReadGeoModel.h>
#include <SHiP/geometry/DetectorBuilder.h>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace SHiP {

// ----------------------------------------------------------------------------
// Factory: build from C++ source
// ----------------------------------------------------------------------------

std::unique_ptr<GeometryService> GeometryService::fromSource() {
    auto svc = std::unique_ptr<GeometryService>(new GeometryService());
    SHiP::geometry::DetectorBuilder builder;
    svc->m_world = builder.build();
    if (!svc->m_world)
        throw std::runtime_error("GeometryService::fromSource: geometry build returned null");
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: load from GeoModel .db file
// ----------------------------------------------------------------------------

std::unique_ptr<GeometryService> GeometryService::fromFile(const std::filesystem::path& dbPath) {
    // GMDBManager opens via sqlite3_open, which silently creates a missing
    // file; validate up front to fail with a clear message instead.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("GeometryService::fromFile: no such file: " + dbPath.string());
    auto svc = std::unique_ptr<GeometryService>(new GeometryService());
    auto db = std::make_shared<GMDBManager>(dbPath.string());
    // An existing but corrupt or non-GeoModel file leaves the manager
    // unopened; ReadGeoModel would fail deep inside library code otherwise.
    if (!db->checkIsDBOpen())
        throw std::runtime_error("GeometryService::fromFile: not a GeoModel database: " +
                                 dbPath.string());
    GeoModelIO::ReadGeoModel reader(db);
    svc->m_world = reader.buildGeoModel();
    if (!svc->m_world)
        throw std::runtime_error("GeometryService::fromFile: buildGeoModel returned null");
    return svc;
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
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("GeometryService::sharedFromFile: no such file: " +
                                 dbPath.string());
    const auto key = std::filesystem::canonical(dbPath).string();

    // The lock is held across the load so concurrent first callers cannot
    // both read the file; contention is limited to job initialisation.
    std::lock_guard lk{registryMutex};
    std::erase_if(registry, [](const auto& entry) { return entry.second.expired(); });
    auto& slot = registry[key];
    if (auto existing = slot.lock())
        return existing;
    std::shared_ptr<GeometryService> svc = fromFile(key);
    slot = svc;
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: wrap an existing GeoModel tree
// ----------------------------------------------------------------------------

std::unique_ptr<GeometryService> GeometryService::fromWorld(PVConstLink world) {
    if (!world)
        throw std::runtime_error("GeometryService::fromWorld: world is null");
    auto svc = std::unique_ptr<GeometryService>(new GeometryService());
    svc->m_world = std::move(world);
    return svc;
}

// ----------------------------------------------------------------------------
// IGeometryService implementation
// ----------------------------------------------------------------------------

const GeoVPhysVol* GeometryService::geoModelWorld() const {
    return m_world.get();
}

namespace {

// Logical volumes form a DAG (daughters are shared across placements);
// the visited set keeps the walk linear in distinct volumes instead of
// re-walking shared subtrees once per placement. Pre-order with emplace
// keeps the first volume of each name, as a depth-first search would.
// Index is GeometryService::m_lvByName, whose type names a private hash.
template <typename Index>
void indexLogicalVolumes(G4LogicalVolume* lv, Index& index,
                         std::unordered_set<const G4LogicalVolume*>& visited) {
    if (!visited.insert(lv).second)
        return;
    index.emplace(lv->GetName(), lv);
    for (auto i = 0uz; i < lv->GetNoDaughters(); ++i) {
        indexLogicalVolumes(lv->GetDaughter(i)->GetLogicalVolume(), index, visited);
    }
}

}  // namespace

G4LogicalVolume& GeometryService::geant4WorldLogical() {
    std::call_once(m_g4ConversionDone, [this]() {
        // A failed conversion is sticky: it can leave partially registered
        // volumes in the Geant4 stores, so a retry could silently produce a
        // mixed tree.
        // Store the error and rethrow it on every call instead.
        try {
            VolumeBuilder builder{"SHiP"};
            m_g4WorldLV = builder.Build(m_world);
            if (!m_g4WorldLV)
                throw std::runtime_error("GeometryService: GeoModel→Geant4 conversion failed");
            std::unordered_set<const G4LogicalVolume*> visited;
            indexLogicalVolumes(m_g4WorldLV, m_lvByName, visited);
        } catch (...) {
            m_conversionError = std::current_exception();
        }
    });
    if (m_conversionError)
        std::rethrow_exception(m_conversionError);
    return *m_g4WorldLV;
}

G4LogicalVolume* GeometryService::getLogicalVolume(std::string_view name) const {
    // A failed conversion must not look like "not converted yet".
    if (m_conversionError)
        std::rethrow_exception(m_conversionError);
    // Index this service's own tree rather than search G4LogicalVolumeStore:
    // retained trees of expired instances of the same file leave
    // identically named volumes in the store, and a store lookup would
    // return the first — stale — match. Empty until conversion.
    const auto it = m_lvByName.find(name);
    return it == m_lvByName.end() ? nullptr : it->second;
}

}  // namespace SHiP
