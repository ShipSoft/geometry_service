// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPMagneticFieldService.h"

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <mp-units/systems/si.h>

using ship::IMagneticFieldService;
using ship::SHiPMagneticFieldService;
namespace si = mp_units::si;
using namespace mp_units::si::unit_symbols;

// ============================================================================
// Geometry reference values (must match SHiPMagneticFieldService.cpp)
// ============================================================================

// MuonShield
constexpr double k_msZ = 16763.3;
constexpr double k_msHX = 1810.0;
constexpr double k_msHY = 1700.0;
constexpr double k_msHZ = 14724.0;

// Spectrometer Magnet
constexpr double k_smZ = 89570.0;
constexpr double k_smHZ = 2500.0;

// ============================================================================
// Tests
// ============================================================================

TEST(MagneticFieldServiceTest, DefaultRegionsCreatesService) {
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    ASSERT_NE(svc, nullptr);
    EXPECT_EQ(svc->numRegions(), 2u);
}

TEST(MagneticFieldServiceTest, FieldNonZeroAtMuonShieldCentre) {
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(0.0 * mm, 0.0 * mm, k_msZ * mm);
    EXPECT_NEAR(field[0].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[1].numerical_value_in(T), 1.8, 1e-12);
    EXPECT_NEAR(field[2].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldNonZeroInsideMuonShield) {
    // Point near the upstream edge but still inside
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    // upstream edge: k_msZ - k_msHZ = 2039.3 mm; use 2040.0 mm (inside)
    auto field = svc->getField(0.0 * mm, 0.0 * mm, 2040.0 * mm);
    EXPECT_NEAR(field[1].numerical_value_in(T), 1.8, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldZeroJustOutsideMuonShieldInZ) {
    // Upstream edge: k_msZ - k_msHZ = 2039.3 mm; 1 mm outside = 2038.3 mm
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(0.0 * mm, 0.0 * mm, 2038.3 * mm);
    EXPECT_NEAR(field[0].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[1].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[2].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldZeroJustOutsideMuonShieldInX) {
    // halfX = 1810.0; 1 mm outside: x = 1811.0 mm
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(1811.0 * mm, 0.0 * mm, k_msZ * mm);
    EXPECT_NEAR(field[1].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldNonZeroAtSpectrometerCentre) {
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(0.0 * mm, 0.0 * mm, k_smZ * mm);
    EXPECT_NEAR(field[0].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[1].numerical_value_in(T), 1.0, 1e-12);
    EXPECT_NEAR(field[2].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldZeroJustOutsideSpectrometerInZ) {
    // Downstream edge: k_smZ + k_smHZ = 92070.0 mm; 1 mm outside = 92071 mm
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(0.0 * mm, 0.0 * mm, 92071.0 * mm);
    EXPECT_NEAR(field[1].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, FieldZeroInDecayVolume) {
    // Decay volume centre: ~58120 mm — between MuonShield and Spectrometer
    auto svc = SHiPMagneticFieldService::withDefaultRegions(1.8, 1.0);
    auto field = svc->getField(0.0 * mm, 0.0 * mm, 58120.0 * mm);
    EXPECT_NEAR(field[0].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[1].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[2].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, CustomEvaluatorRespected) {
    // Build a single region covering a known box with a custom Bx evaluator
    SHiPMagneticFieldService::FieldRegion r;
    r.centreX = 0.0;
    r.centreY = 0.0;
    r.centreZ = 0.0;
    r.halfX = 1000.0;
    r.halfY = 1000.0;
    r.halfZ = 1000.0;
    r.evaluator = [](double, double, double) -> std::array<double, 3> { return {0.5, 0.0, 0.0}; };

    std::vector<SHiPMagneticFieldService::FieldRegion> regions;
    regions.push_back(std::move(r));
    SHiPMagneticFieldService svc(std::move(regions));

    // Inside: Bx = 0.5 T
    auto inside = svc.getField(0.0 * mm, 0.0 * mm, 0.0 * mm);
    EXPECT_NEAR(inside[0].numerical_value_in(T), 0.5, 1e-12);
    EXPECT_NEAR(inside[1].numerical_value_in(T), 0.0, 1e-12);

    // Outside: all zero
    auto outside = svc.getField(1001.0 * mm, 0.0 * mm, 0.0 * mm);
    EXPECT_NEAR(outside[0].numerical_value_in(T), 0.0, 1e-12);
}

TEST(MagneticFieldServiceTest, EmptyRegionListAlwaysZero) {
    SHiPMagneticFieldService svc({});
    auto field = svc.getField(0.0 * mm, 0.0 * mm, k_msZ * mm);
    EXPECT_NEAR(field[0].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[1].numerical_value_in(T), 0.0, 1e-12);
    EXPECT_NEAR(field[2].numerical_value_in(T), 0.0, 1e-12);
}
