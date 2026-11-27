// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// The core header must stay free of GeoModel and Geant4. Every GeoModel
// kernel header pulls in GeoIntrusivePtr.h, and every Geant4 header G4Types.hh.
#include "SHiP/GeometryService/GeometryService.h"

#ifdef GEOMODELKERNEL_GeoIntrusivePtr_H
#error "GeometryService.h includes GeoModel headers"
#endif
#ifdef G4TYPES_HH
#error "GeometryService.h includes Geant4 headers"
#endif

#include <catch2/catch_test_macros.hpp>

TEST_CASE("CoreHeaders.FreeOfGeoModelAndGeant4", "[geometry_service]") {
    SUCCEED("checked at compile time");
}
