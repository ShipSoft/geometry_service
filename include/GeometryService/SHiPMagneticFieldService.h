// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "GeometryService/IMagneticFieldService.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace ship {

/// Concrete implementation of IMagneticFieldService using axis-aligned bounding-box regions.
///
/// Each FieldRegion covers a rectangular box (defined by centre + half-widths in mm)
/// and carries an evaluator function that returns {Bx, By, Bz} in Tesla for any point
/// inside that box. Fields from overlapping regions are summed.
///
/// The evaluator pattern is the covfie bridge: a future field-map backend replaces the
/// constant lambda with a covfie view capture without any interface change.
class SHiPMagneticFieldService final : public IMagneticFieldService {
public:
    struct FieldRegion {
        double centreX{0.0};  ///< Box centre X [mm]
        double centreY{0.0};  ///< Box centre Y [mm]
        double centreZ{0.0};  ///< Box centre Z [mm]
        double halfX{0.0};    ///< Half-width in X [mm]
        double halfY{0.0};    ///< Half-width in Y [mm]
        double halfZ{0.0};    ///< Half-width in Z [mm]
        /// Returns {Bx, By, Bz} in Tesla at position (x, y, z) [mm] (position unused for uniform fields).
        std::function<std::array<double, 3>(double, double, double)> evaluator;
    };

    explicit SHiPMagneticFieldService(std::vector<FieldRegion> regions);

    ~SHiPMagneticFieldService() override = default;

    // Non-copyable, movable
    SHiPMagneticFieldService(const SHiPMagneticFieldService&) = delete;
    SHiPMagneticFieldService& operator=(const SHiPMagneticFieldService&) = delete;
    SHiPMagneticFieldService(SHiPMagneticFieldService&&) = default;
    SHiPMagneticFieldService& operator=(SHiPMagneticFieldService&&) = default;

    /// Factory: build the standard SHiP detector field regions.
    /// @param muonShieldFieldT      Uniform By in the MuonShield region [T] (default 1.8 T)
    /// @param spectrometerFieldT    Uniform By in the Spectrometer Magnet region [T] (default 1.0 T)
    static std::unique_ptr<SHiPMagneticFieldService>
    withDefaultRegions(double muonShieldFieldT = 1.8, double spectrometerFieldT = 1.0);

    /// Number of field regions (for testing / diagnostics).
    [[nodiscard]] std::size_t numRegions() const noexcept { return m_regions.size(); }

    [[nodiscard]] std::array<field_q, 3>
    getField(pos_q x, pos_q y, pos_q z) const override;

private:
    std::vector<FieldRegion> m_regions;
};

}  // namespace ship
