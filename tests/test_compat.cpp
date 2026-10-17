// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Pre-0.5.0 spellings, kept working until 0.6.0: the old header paths, the
// `ship` namespace alias and the SHiPGeometryService name. Delete this file
// together with the forwarders in include/GeometryService/.

#include "GeometryService/G4RayScanner.h"
#include "GeometryService/GeometryThread.h"
#include "GeometryService/IGeometryService.h"
#include "GeometryService/SHiPGeometryService.h"

#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

static_assert(std::is_same_v<ship::SHiPGeometryService, SHiP::GeometryService>);
static_assert(std::is_same_v<ship::IGeometryService, SHiP::IGeometryService>);
static_assert(std::is_same_v<ship::G4RayScanner, SHiP::G4RayScanner>);
static_assert(std::is_same_v<ship::GeometryThread, SHiP::GeometryThread>);

TEST_CASE("Compat.OldSpellingsReachTheService", "[compat]") {
    CHECK_THROWS_AS(ship::SHiPGeometryService::fromWorld(nullptr), std::runtime_error);
    CHECK(&ship::geometry_thread() == &SHiP::geometry_thread());
}
