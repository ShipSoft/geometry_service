// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/GeoModel.h"
#include "test_world.h"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

TEST_CASE("GeoModelInterop.FromWorldNullThrows", "[geomodel]") {
    CHECK_THROWS_AS(SHiP::geometryServiceFromWorld(nullptr), std::runtime_error);
}

TEST_CASE("GeoModelInterop.WorldRoundTrips", "[geomodel]") {
    const PVConstLink world = buildTestWorld();
    const auto svc = SHiP::geometryServiceFromWorld(world);
    REQUIRE(svc != nullptr);
    CHECK(SHiP::geoModelWorld(*svc) == world);
}

TEST_CASE("GeoModelInterop.WorldOutlivesService", "[geomodel]") {
    PVConstLink world;
    {
        const auto svc = SHiP::geometryServiceFromWorld(buildTestWorld());
        world = SHiP::geoModelWorld(*svc);
    }
    REQUIRE(world);
    CHECK(world->getLogVol()->getName() == "TestWorld");
}
