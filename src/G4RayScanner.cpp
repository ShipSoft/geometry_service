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
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

namespace ship {

G4RayScanner::G4RayScanner(G4LogicalVolume* top) {
    if (!top)
        throw std::invalid_argument("G4RayScanner: top logical volume is null");
    // G4GeometryManager's closed state is one process-wide flag, not
    // per-tree: closing while already closed (another scanner alive, or a
    // run manager mid-run) would be a silent no-op and this scanner would
    // navigate without voxelisation. Fail loud instead.
    if (G4GeometryManager::GetInstance()->IsGeometryClosed())
        throw std::logic_error(
            "G4RayScanner: Geant4 geometry is already closed "
            "(another G4RayScanner alive, or a run in progress)");
    // Owned locally until construction succeeds: the destructor does not run
    // after a throw, and the placement has already registered itself in the
    // physical-volume store.
    auto world = std::make_unique<G4PVPlacement>(
        nullptr, G4ThreeVector(), top, top->GetName() + "_rayscan_world", nullptr, false, 0);
    m_navigator.SetWorldVolume(world.get());
    // Voxelise only this tree; a full CloseGeometry() would also close any
    // other geometry living in the same process. Last, so nothing that runs
    // after it can throw and leave the geometry closed.
    G4GeometryManager::GetInstance()->CloseGeometry(/*pOptimise=*/true, /*verbose=*/false,
                                                    world.get());
    m_world = world.release();
}

G4RayScanner::~G4RayScanner() {
    // If another Geant4 user (e.g. a run manager shutdown) already cleaned
    // the stores, the placement is gone and must not be touched.
    auto* store = G4PhysicalVolumeStore::GetInstance();
    if (std::find(store->begin(), store->end(), m_world) == store->end())
        return;
    // Deopt only while the geometry is closed (OpenGeometry is a no-op
    // otherwise). Caveat: the closed flag is global, so if a run manager
    // re-closed the full geometry meanwhile, this also flips its state open.
    if (G4GeometryManager::GetInstance()->IsGeometryClosed())
        G4GeometryManager::GetInstance()->OpenGeometry(m_world);
    delete m_world;
}

RayScan G4RayScanner::scan(const G4ThreeVector& origin, const G4ThreeVector& direction) const {
    // unit() hands a null vector back unchanged, and non-finite components
    // would poison every step: reject both instead of reporting a miss.
    // Check the components rather than mag2(), which overflows or underflows
    // for very large or very small but valid directions, and divide by the
    // largest component before normalising for the same reason.
    const double scale =
        std::max({std::abs(direction.x()), std::abs(direction.y()), std::abs(direction.z())});
    if (!std::isfinite(direction.x()) || !std::isfinite(direction.y()) ||
        !std::isfinite(direction.z()) || scale == 0.)
        throw std::invalid_argument("G4RayScanner::scan: direction must be finite and non-zero");
    const G4ThreeVector dir = (direction / scale).unit();
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
    // Checked before stepping, so a ray that exits on the last allowed step
    // still returns normally while a truncated scan never looks like one.
    constexpr int kMaxSteps = 100000;
    for (int steps = 0; volume; ++steps) {
        if (steps == kMaxSteps)
            throw std::runtime_error("G4RayScanner::scan: step limit reached (navigation loop?)");
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
