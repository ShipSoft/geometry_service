// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/G4RayScanner.h"
#include "GeometryService/SHiPGeometryService.h"
#include "test_world.h"

#include <G4LogicalVolume.hh>
#include <G4Material.hh>

#include <CLHEP/Units/SystemOfUnits.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>

using Catch::Matchers::WithinAbs;
using ship::G4RayScanner;
using ship::SHiPGeometryService;

namespace {

// One service and G4 conversion shared by all tests: converted volumes live
// in the global Geant4 stores, so repeated conversions would pile up copies.
SHiPGeometryService& service() {
    static std::unique_ptr<SHiPGeometryService> svc = [] {
        auto s = SHiPGeometryService::fromWorld(buildTestWorld());
        s->geant4WorldLogical();
        return s;
    }();
    return *svc;
}

G4RayScanner& worldScanner() {
    static G4RayScanner scanner{service().geant4WorldLogical()};
    return scanner;
}

}  // namespace

TEST_CASE("RayScannerTest.AxialRayFromOutsideCrossesAllVolumes", "[ray_scanner]") {
    auto [entry, segments] = worldScanner().scan({0, 0, -1500}, {0, 0, 1});
    CHECK_THAT(entry, WithinAbs(500., 1e-6));  // world entry jump to z = -1000
    REQUIRE(segments.size() == 5U);
    CHECK(segments[0].material->GetName() == "TestGas");
    CHECK(segments[1].material->GetName() == "TestCopper");
    CHECK(segments[2].material->GetName() == "TestGas");
    CHECK(segments[3].material->GetName() == "TestIron");
    CHECK(segments[4].material->GetName() == "TestGas");
    CHECK_THAT(segments[0].length, WithinAbs(500., 1e-6));  // world entry jump to -1000
    CHECK_THAT(segments[1].length, WithinAbs(200., 1e-6));
    CHECK_THAT(segments[2].length, WithinAbs(400., 1e-6));
    CHECK_THAT(segments[3].length, WithinAbs(200., 1e-6));
    CHECK_THAT(segments[4].length, WithinAbs(700., 1e-6));
}

TEST_CASE("RayScannerTest.RayFromInsideStartsAtOrigin", "[ray_scanner]") {
    auto [entry, segments] = worldScanner().scan({0, 0, 0}, {0, 0, 1});
    CHECK(entry == 0.);
    REQUIRE(segments.size() == 3U);
    CHECK(segments[0].material->GetName() == "TestGas");
    CHECK(segments[1].material->GetName() == "TestIron");
    CHECK(segments[2].material->GetName() == "TestGas");
    CHECK_THAT(segments[0].length, WithinAbs(100., 1e-6));
    CHECK_THAT(segments[1].length, WithinAbs(200., 1e-6));
    CHECK_THAT(segments[2].length, WithinAbs(700., 1e-6));
}

TEST_CASE("RayScannerTest.UnnormalisedDirectionIsAccepted", "[ray_scanner]") {
    auto [entry, segments] = worldScanner().scan({0, 0, 0}, {0, 0, 42.});
    CHECK(entry == 0.);
    REQUIRE(segments.size() == 3U);
    CHECK_THAT(segments[1].length, WithinAbs(200., 1e-6));
}

TEST_CASE("RayScannerTest.MissingRayReturnsEmpty", "[ray_scanner]") {
    CHECK(worldScanner().scan({0, 5000, -1500}, {0, 0, 1}).segments.empty());
    // Pointing away from the world.
    CHECK(worldScanner().scan({0, 0, -1500}, {0, 0, -1}).segments.empty());
}

TEST_CASE("RayScannerTest.DaughterVolumeScanUsesItsLocalFrame", "[ray_scanner]") {
    // The iron cube sits at z = +200 mm in the world, but a scanner built on
    // its logical volume works in the cube's own frame, centred on origin.
    auto* ironLV = service().getLogicalVolume("TestIronBox");
    REQUIRE(ironLV != nullptr);
    G4RayScanner scanner{ironLV};
    auto [entry, segments] = scanner.scan({0, 0, -500}, {0, 0, 1});
    CHECK_THAT(entry, WithinAbs(400., 1e-6));
    REQUIRE(segments.size() == 1U);
    CHECK(segments[0].material->GetName() == "TestIron");
    CHECK_THAT(segments[0].length, WithinAbs(2 * kCubeHalf, 1e-6));
}

TEST_CASE("RayScannerTest.ConvertedDensitiesMatchGeoModel", "[ray_scanner]") {
    // Guards GeoModel2G4 density unit handling end to end.
    auto [entry, segments] = worldScanner().scan({0, 0, -1500}, {0, 0, 1});
    (void)entry;
    REQUIRE(segments.size() == 5U);
    CHECK_THAT(segments[0].material->GetDensity() / (CLHEP::g / CLHEP::cm3),
               WithinAbs(kGasDensity, 1e-6));
    CHECK_THAT(segments[1].material->GetDensity() / (CLHEP::g / CLHEP::cm3),
               WithinAbs(kCopperDensity, 1e-6));
    CHECK_THAT(segments[3].material->GetDensity() / (CLHEP::g / CLHEP::cm3),
               WithinAbs(kIronDensity, 1e-6));
}

TEST_CASE("RayScannerTest.NullTopVolumeThrows", "[ray_scanner]") {
    CHECK_THROWS_AS(G4RayScanner{nullptr}, std::invalid_argument);
}
