// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <gtest/gtest.h>

#include <memory>

using ship::SHiPGeometryService;

TEST(GeometryServiceTest, FromSourceReturnsNonNull) {
    auto svc = SHiPGeometryService::fromSource();
    EXPECT_NE(svc, nullptr);
}

TEST(GeometryServiceTest, GeoModelWorldNonNull) {
    auto svc = SHiPGeometryService::fromSource();
    ASSERT_NE(svc, nullptr);
    EXPECT_NE(svc->geoModelWorld(), nullptr);
}

TEST(GeometryServiceTest, GetLogicalVolumeBeforeG4ReturnsNull) {
    // geant4WorldLogical() has not been called, so G4LogicalVolumeStore is empty.
    // getLogicalVolume() must return nullptr without crashing.
    auto svc = SHiPGeometryService::fromSource();
    ASSERT_NE(svc, nullptr);
    EXPECT_EQ(svc->getLogicalVolume("anything"), nullptr);
}
