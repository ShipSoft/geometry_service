// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

using ship::SHiPGeometryService;

TEST_CASE("GeometryServiceTest.FromFileMissingFileThrows", "[geometry_service]") {
    CHECK_THROWS_AS(SHiPGeometryService::fromFile("/nonexistent/path.db"), std::runtime_error);
}

TEST_CASE("GeometryServiceTest.FromSourceReturnsNonNull", "[geometry_service]") {
    auto svc = SHiPGeometryService::fromSource();
    CHECK(svc != nullptr);
}

TEST_CASE("GeometryServiceTest.GeoModelWorldNonNull", "[geometry_service]") {
    auto svc = SHiPGeometryService::fromSource();
    REQUIRE(svc != nullptr);
    CHECK(svc->geoModelWorld() != nullptr);
}

TEST_CASE("GeometryServiceTest.GetLogicalVolumeBeforeG4ReturnsNull", "[geometry_service]") {
    // geant4WorldLogical() has not been called, so G4LogicalVolumeStore is empty.
    // getLogicalVolume() must return nullptr without crashing.
    auto svc = SHiPGeometryService::fromSource();
    REQUIRE(svc != nullptr);
    CHECK(svc->getLogicalVolume("anything") == nullptr);
}
