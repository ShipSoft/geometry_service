// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/G4RayScanner.h"

#include <G4GeometryManager.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4VSolid.hh>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace ship {

G4RayScanner::G4RayScanner(G4LogicalVolume* top) {
    if (!top)
        throw std::invalid_argument("G4RayScanner: top logical volume is null");
    m_world = new G4PVPlacement(nullptr, G4ThreeVector(), top, top->GetName() + "_rayscan_world",
                                nullptr, false, 0);
    // Voxelise only this tree; a full CloseGeometry() would also close any
    // other geometry living in the same process.
    G4GeometryManager::GetInstance()->CloseGeometry(/*pOptimise=*/true, /*verbose=*/false, m_world);
    m_navigator.SetWorldVolume(m_world);
}

G4RayScanner::~G4RayScanner() {
    // If another Geant4 user (e.g. a run manager shutdown) already cleaned
    // the stores, the placement is gone and must not be touched.
    auto* store = G4PhysicalVolumeStore::GetInstance();
    if (std::find(store->begin(), store->end(), m_world) == store->end())
        return;
    G4GeometryManager::GetInstance()->OpenGeometry(m_world);
    delete m_world;
}

RayScan G4RayScanner::scan(const G4ThreeVector& origin, const G4ThreeVector& direction) const {
    const G4ThreeVector dir = direction.unit();
    G4ThreeVector point = origin;
    RayScan result;

    // Flux rays typically start far outside the scanned volume: jump straight
    // to the entry point instead of stepping through the void.
    const G4VSolid* solid = m_world->GetLogicalVolume()->GetSolid();
    if (solid->Inside(point) == kOutside) {
        const double entry = solid->DistanceToIn(point, dir);
        if (entry == kInfinity)
            return result;
        point += entry * dir;
        result.entry = entry;
    }

    auto& segments = result.segments;
    m_navigator.ResetStackAndState();
    // Direction-aware locate so a point on a boundary resolves to the volume
    // ahead of the ray.
    G4VPhysicalVolume* volume = m_navigator.LocateGlobalPointAndSetup(
        point, &dir, /*pRelativeSearch=*/false, /*ignoreDirection=*/false);
    // Boundary count is bounded by the geometry's depth times its daughters;
    // anything near the cap indicates a navigation loop, not a real geometry.
    constexpr int max_steps = 100000;
    for (int i = 0; volume && i < max_steps; ++i) {
        double safety = 0.;
        const double step = m_navigator.ComputeStep(point, dir, kInfinity, safety);
        if (step == kInfinity)
            break;  // no boundary ahead (world exit edge case)
        if (step > 0.) {
            const G4Material* material = volume->GetLogicalVolume()->GetMaterial();
            if (!segments.empty() && segments.back().material == material)
                segments.back().length += step;
            else
                segments.push_back({material, step});
        }
        point += step * dir;
        m_navigator.SetGeometricallyLimitedStep();
        volume = m_navigator.LocateGlobalPointAndSetup(point, &dir, /*pRelativeSearch=*/true,
                                                       /*ignoreDirection=*/false);
    }
    return result;
}

}  // namespace ship
