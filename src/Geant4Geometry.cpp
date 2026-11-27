// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/Geant4Geometry.h"

#include "SHiP/GeometryService/GeoModel.h"

#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>

#include <GeoModel2G4/VolumeBuilder.h>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace SHiP {

std::shared_ptr<Geant4Geometry> Geant4Geometry::of(std::shared_ptr<const GeometryService> service) {
    if (!service) {
        throw std::invalid_argument("Geant4Geometry::of: service is null");
    }
    static std::mutex registryMutex;
    // Keyed by address. A live entry holds its service, so the address
    // cannot be reused while the entry is live; expired entries are dropped.
    static std::map<const GeometryService*, std::weak_ptr<Geant4Geometry>> registry;

    const std::scoped_lock lk{registryMutex};
    std::erase_if(registry, [](const auto& entry) { return entry.second.expired(); });
    auto& slot = registry[service.get()];
    if (auto existing = slot.lock()) {
        return existing;
    }
    std::shared_ptr<Geant4Geometry> view{new Geant4Geometry(std::move(service))};
    slot = view;
    return view;
}

Geant4Geometry::Geant4Geometry(std::shared_ptr<const GeometryService> service)
    : m_service{std::move(service)} {}

Geant4Geometry::~Geant4Geometry() = default;

namespace {

// Logical volumes form a DAG (daughters are shared across placements);
// the visited set keeps the walk linear in distinct volumes instead of
// re-walking shared subtrees once per placement. Pre-order with emplace
// keeps the first volume of each name, as a depth-first search would.
// Index is Geant4Geometry::m_lvByName, whose type names a private hash.
template <typename Index>
void indexLogicalVolumes(G4LogicalVolume* lv, Index& index,
                         std::unordered_set<const G4LogicalVolume*>& visited) {
    if (!visited.insert(lv).second) {
        return;
    }
    index.emplace(lv->GetName(), lv);
    for (auto i = 0uz; i < lv->GetNoDaughters(); ++i) {
        indexLogicalVolumes(lv->GetDaughter(i)->GetLogicalVolume(), index, visited);
    }
}

}  // namespace

G4LogicalVolume& Geant4Geometry::worldLogical() {
    std::call_once(m_conversionDone, [this] {
        // A failed conversion is sticky: it can leave partially registered
        // volumes in the Geant4 stores, so a retry could silently produce a
        // mixed tree.
        // Store the error and rethrow it on every call instead.
        try {
            VolumeBuilder builder{"SHiP"};
            m_worldLV = builder.Build(geoModelWorld(*m_service));
            if (!m_worldLV) {
                throw std::runtime_error("Geant4Geometry: GeoModel→Geant4 conversion failed");
            }
            std::unordered_set<const G4LogicalVolume*> visited;
            indexLogicalVolumes(m_worldLV, m_lvByName, visited);
        } catch (...) {
            m_conversionError = std::current_exception();
        }
    });
    if (m_conversionError) {
        std::rethrow_exception(m_conversionError);
    }
    return *m_worldLV;
}

G4LogicalVolume* Geant4Geometry::logicalVolume(std::string_view name) const {
    // A failed conversion must not look like "not converted yet".
    if (m_conversionError) {
        std::rethrow_exception(m_conversionError);
    }
    // Index this view's own tree rather than search G4LogicalVolumeStore:
    // retained trees of expired views of the same file leave identically
    // named volumes in the store, and a store lookup would return the
    // first, stale, match. Empty until conversion.
    const auto it = m_lvByName.find(name);
    return it == m_lvByName.end() ? nullptr : it->second;
}

}  // namespace SHiP
