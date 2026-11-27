// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/Geant4Geometry.h"
#include "SHiP/GeometryService/GeoModel.h"
#include "test_world.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

using SHiP::Geant4Geometry;

TEST_CASE("Geant4GeometryTest.NullServiceThrows", "[geant4_geometry]") {
    CHECK_THROWS_AS(Geant4Geometry::of(nullptr), std::invalid_argument);
}

TEST_CASE("Geant4GeometryTest.OneViewPerService", "[geant4_geometry]") {
    const std::shared_ptr<const SHiP::GeometryService> svc =
        SHiP::geometryServiceFromWorld(buildTestWorld());
    const auto first = Geant4Geometry::of(svc);
    const auto second = Geant4Geometry::of(svc);
    CHECK(first == second);
    CHECK(&first->service() == svc.get());

    const auto other = Geant4Geometry::of(SHiP::geometryServiceFromWorld(buildTestWorld()));
    CHECK(other != first);
}

TEST_CASE("Geant4GeometryTest.ViewKeepsServiceAlive", "[geant4_geometry]") {
    std::shared_ptr<const SHiP::GeometryService> svc =
        SHiP::geometryServiceFromWorld(buildTestWorld());
    const std::weak_ptr<const SHiP::GeometryService> weak{svc};
    const auto view = Geant4Geometry::of(std::move(svc));
    CHECK_FALSE(weak.expired());
}

TEST_CASE("Geant4GeometryTest.LogicalVolumeBeforeConversionReturnsNull", "[geant4_geometry]") {
    // worldLogical() has not been called, so there is no converted tree yet.
    const auto view = Geant4Geometry::of(SHiP::geometryServiceFromWorld(buildTestWorld()));
    CHECK(view->logicalVolume("anything") == nullptr);
}
