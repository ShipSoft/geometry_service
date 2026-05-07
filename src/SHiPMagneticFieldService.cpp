// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPMagneticFieldService.h"

#include <mp-units/systems/si.h>

#include <cmath>
#include <memory>
#include <stdexcept>

namespace ship {

// ---------------------------------------------------------------------------
// Geometry constants (all dimensions in mm, matching SHiPGeometry.cpp)
// ---------------------------------------------------------------------------

namespace {

// MuonShield bounding box
constexpr double k_muonShieldCentreZ = 16763.3;
constexpr double k_muonShieldHalfX   = 1810.0;
constexpr double k_muonShieldHalfY   = 1700.0;
constexpr double k_muonShieldHalfZ   = 14724.0;

// Spectrometer Magnet bounding box  (SHiPGeometry: 89.57 m centre)
constexpr double k_spectrometerCentreZ = 89570.0;
constexpr double k_spectrometerHalfX   = 3250.0;
constexpr double k_spectrometerHalfY   = 4300.0;
constexpr double k_spectrometerHalfZ   = 2500.0;

}  // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

SHiPMagneticFieldService::SHiPMagneticFieldService(std::vector<FieldRegion> regions)
    : m_regions(std::move(regions)) {}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<SHiPMagneticFieldService>
SHiPMagneticFieldService::withDefaultRegions(double muonShieldFieldT,
                                              double spectrometerFieldT) {
    std::vector<FieldRegion> regions;
    regions.reserve(2);

    // ── MuonShield ─────────────────────────────────────────────────────────
    {
        FieldRegion r;
        r.centreX = 0.0;
        r.centreY = 0.0;
        r.centreZ = k_muonShieldCentreZ;
        r.halfX   = k_muonShieldHalfX;
        r.halfY   = k_muonShieldHalfY;
        r.halfZ   = k_muonShieldHalfZ;
        const double By = muonShieldFieldT;
        r.evaluator = [By](double, double, double) -> std::array<double, 3> {
            return {0.0, By, 0.0};
        };
        regions.push_back(std::move(r));
    }

    // ── Spectrometer Magnet ────────────────────────────────────────────────
    {
        FieldRegion r;
        r.centreX = 0.0;
        r.centreY = 0.0;
        r.centreZ = k_spectrometerCentreZ;
        r.halfX   = k_spectrometerHalfX;
        r.halfY   = k_spectrometerHalfY;
        r.halfZ   = k_spectrometerHalfZ;
        const double By = spectrometerFieldT;
        r.evaluator = [By](double, double, double) -> std::array<double, 3> {
            return {0.0, By, 0.0};
        };
        regions.push_back(std::move(r));
    }

    return std::make_unique<SHiPMagneticFieldService>(std::move(regions));
}

// ---------------------------------------------------------------------------
// getField
// ---------------------------------------------------------------------------

std::array<IMagneticFieldService::field_q, 3>
SHiPMagneticFieldService::getField(pos_q x, pos_q y, pos_q z) const {
    using namespace mp_units::si;

    const double x_mm = x.numerical_value_in(milli<metre>);
    const double y_mm = y.numerical_value_in(milli<metre>);
    const double z_mm = z.numerical_value_in(milli<metre>);

    double Bx = 0.0, By = 0.0, Bz = 0.0;

    for (const auto& r : m_regions) {
        if (std::abs(x_mm - r.centreX) <= r.halfX &&
            std::abs(y_mm - r.centreY) <= r.halfY &&
            std::abs(z_mm - r.centreZ) <= r.halfZ) {
            auto v = r.evaluator(x_mm, y_mm, z_mm);
            Bx += v[0];
            By += v[1];
            Bz += v[2];
        }
    }

    return {Bx * tesla, By * tesla, Bz * tesla};
}

}  // namespace ship
