// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// In its own executable: emptying the Geant4 stores invalidates every
// converted geometry in the process.

#include "SHiP/GeometryService/GeometryService.h"
#include "SHiP/GeometryService/GeometryThread.h"
#include "test_world.h"

#include <G4LogicalVolumeStore.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4SolidStore.hh>

#include <catch2/catch_test_macros.hpp>
#include <cstddef>

TEST_CASE("CleanGeant4GeometryStores.EmptiesTheStores", "[geometry_thread]") {
    auto service = SHiP::GeometryService::fromWorld(buildTestWorld());

    struct Sizes {
        std::size_t logical;
        std::size_t physical;
        std::size_t solids;
    };
    const auto sizes = [] {
        return SHiP::geometry_thread().run([] {
            return Sizes{.logical = G4LogicalVolumeStore::GetInstance()->size(),
                         .physical = G4PhysicalVolumeStore::GetInstance()->size(),
                         .solids = G4SolidStore::GetInstance()->size()};
        });
    };

    SHiP::geometry_thread().run([&] { service->geant4WorldLogical(); });
    const auto before = sizes();
    // The world and the two cubes.
    CHECK(before.logical == 3);
    CHECK(before.physical >= 2);
    CHECK(before.solids >= 1);

    SHiP::clean_geant4_geometry_stores();
    const auto after = sizes();
    CHECK(after.logical == 0);
    CHECK(after.physical == 0);
    CHECK(after.solids == 0);

    // Callable from the geometry thread itself, and on empty stores.
    SHiP::geometry_thread().run([] { SHiP::clean_geant4_geometry_stores(); });
}
