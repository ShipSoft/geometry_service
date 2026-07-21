// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

using ship::SHiPGeometryService;

TEST_CASE("GeometryServiceTest.FromFileMissingFileThrows", "[geometry_service]") {
    CHECK_THROWS_AS(SHiPGeometryService::fromFile("/nonexistent/path.db"), std::runtime_error);
}

TEST_CASE("GeometryServiceTest.FromFileCorruptFileThrows", "[geometry_service]") {
    auto path = std::filesystem::temp_directory_path() /
                ("shipgeomsvc_corrupt_" + std::to_string(getpid()) + ".db");
    {
        std::ofstream out{path};
        out << "not a sqlite database";
    }
    CHECK_THROWS_AS(SHiPGeometryService::fromFile(path), std::runtime_error);
    std::filesystem::remove(path);
}

TEST_CASE("GeometryServiceTest.FromWorldNullThrows", "[geometry_service]") {
    CHECK_THROWS_AS(SHiPGeometryService::fromWorld(nullptr), std::runtime_error);
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
    // geant4WorldLogical() has not been called, so there is no converted
    // tree yet. getLogicalVolume() must return nullptr without crashing.
    auto svc = SHiPGeometryService::fromSource();
    REQUIRE(svc != nullptr);
    CHECK(svc->getLogicalVolume("anything") == nullptr);
}
