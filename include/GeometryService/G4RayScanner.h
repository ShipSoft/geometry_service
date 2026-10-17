// SPDX-FileCopyrightText: CERN for the benefit of the SHiP Collaboration
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Forwarding header for the pre-0.5.0 layout, removed in 0.6.0.
// GeometryService/G4RayScanner.h is now SHiP/GeometryService/G4RayScanner.h, and the types it
// declares moved from the `ship` namespace to `SHiP`.
#pragma once

#include "SHiP/GeometryService/G4RayScanner.h"

#ifndef SHIP_NO_DEPRECATED_NAMESPACE_ALIAS
namespace ship = SHiP;
#endif
