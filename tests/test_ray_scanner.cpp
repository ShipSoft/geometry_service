// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/G4RayScanner.h"
#include "GeometryService/SHiPGeometryService.h"

#include <G4LogicalVolume.hh>
#include <G4Material.hh>

#include <CLHEP/Units/SystemOfUnits.h>
#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoElement.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>

using Catch::Matchers::WithinAbs;
using ship::G4RayScanner;
using ship::SHiPGeometryService;

namespace {

// 2 m gas-filled world box with two 20 cm metal cubes on the z axis:
// copper at z in [-500, -300] mm, iron at z in [100, 300] mm.
constexpr double kWorldHalf = 1000.;     // mm
constexpr double kCubeHalf = 100.;       // mm
constexpr double kIronZ = 200.;          // mm
constexpr double kCopperZ = -400.;       // mm
constexpr double kGasDensity = 1.29e-3;  // g/cm3
constexpr double kIronDensity = 7.874;   // g/cm3
constexpr double kCopperDensity = 8.96;  // g/cm3

PVConstLink buildTestWorld() {
    namespace GU = GeoModelKernelUnits;
    auto* nitrogen = new GeoElement("Nitrogen", "N", 7, 14.007 * GU::g / GU::mole);
    auto* oxygen = new GeoElement("Oxygen", "O", 8, 15.999 * GU::g / GU::mole);
    auto* ironElement = new GeoElement("Iron", "Fe", 26, 55.845 * GU::g / GU::mole);
    auto* copperElement = new GeoElement("Copper", "Cu", 29, 63.546 * GU::g / GU::mole);

    auto* gas = new GeoMaterial("TestGas", kGasDensity * GU::g / GU::cm3);
    gas->add(nitrogen, 0.77);
    gas->add(oxygen, 0.23);
    gas->lock();
    auto* iron = new GeoMaterial("TestIron", kIronDensity * GU::g / GU::cm3);
    iron->add(ironElement, 1.0);
    iron->lock();
    auto* copper = new GeoMaterial("TestCopper", kCopperDensity * GU::g / GU::cm3);
    copper->add(copperElement, 1.0);
    copper->lock();

    auto* world = new GeoPhysVol(
        new GeoLogVol("TestWorld", new GeoBox(kWorldHalf, kWorldHalf, kWorldHalf), gas));
    world->add(new GeoTransform(GeoTrf::Translate3D(0, 0, kIronZ)));
    world->add(new GeoPhysVol(
        new GeoLogVol("TestIronBox", new GeoBox(kCubeHalf, kCubeHalf, kCubeHalf), iron)));
    world->add(new GeoTransform(GeoTrf::Translate3D(0, 0, kCopperZ)));
    world->add(new GeoPhysVol(
        new GeoLogVol("TestCopperBox", new GeoBox(kCubeHalf, kCubeHalf, kCubeHalf), copper)));
    return world;
}

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
