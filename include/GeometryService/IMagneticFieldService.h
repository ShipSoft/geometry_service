// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <mp-units/systems/si.h>

#include <array>

namespace ship {

/// Framework-agnostic interface to the SHiP magnetic field.
class IMagneticFieldService {
public:
    using pos_q   = mp_units::quantity<mp_units::si::milli<mp_units::si::metre>, double>;
    using field_q = mp_units::quantity<mp_units::si::tesla, double>;

    virtual ~IMagneticFieldService() = default;

    /// Return {Bx, By, Bz} in Tesla at position (x, y, z) in millimetres.
    [[nodiscard]] virtual std::array<field_q, 3>
    getField(pos_q x, pos_q y, pos_q z) const = 0;
};

}  // namespace ship
