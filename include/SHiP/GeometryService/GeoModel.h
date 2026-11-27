// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// GeoModel interop for SHiP::GeometryService. Link
// SHiP::GeometryServiceGeoModel to use it. Code that only loads and passes
// the geometry around should stay on GeometryService.h.

#include "SHiP/GeometryService/GeometryService.h"

#include <GeoModelKernel/GeoVPhysVol.h>
#include <memory>

namespace SHiP {

/// The GeoModel world physical volume. The tree is immutable and shared:
/// holding the returned link keeps it alive beyond the service.
[[nodiscard]] PVConstLink geoModelWorld(const GeometryService& service);

/// Wrap an already-built GeoModel world (e.g. a small test geometry, or one
/// built in memory with SHiP::geometry::DetectorBuilder). Throws
/// std::runtime_error on a null world.
[[nodiscard]] std::unique_ptr<GeometryService> geometryServiceFromWorld(PVConstLink world);

}  // namespace SHiP
