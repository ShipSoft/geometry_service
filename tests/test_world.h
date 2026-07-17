// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoElement.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

// 2 m gas-filled world box with two 20 cm metal cubes on the z axis:
// copper at z in [-500, -300] mm, iron at z in [100, 300] mm.
inline constexpr double kWorldHalf = 1000.;     // mm
inline constexpr double kCubeHalf = 100.;       // mm
inline constexpr double kIronZ = 200.;          // mm
inline constexpr double kCopperZ = -400.;       // mm
inline constexpr double kGasDensity = 1.29e-3;  // g/cm3
inline constexpr double kIronDensity = 7.874;   // g/cm3
inline constexpr double kCopperDensity = 8.96;  // g/cm3

inline PVConstLink buildTestWorld() {
    namespace GU = GeoModelKernelUnits;
    auto* nitrogen = new GeoElement("Nitrogen", "N", 7, 14.007 * GU::g / GU::mole);
    auto* oxygen = new GeoElement("Oxygen", "O", 8, 15.999 * GU::g / GU::mole);
    auto* ironElement = new GeoElement("Iron", "Fe", 26, 55.845 * GU::g / GU::mole);
    auto* copperElement = new GeoElement("Copper", "Cu", 29, 63.546 * GU::g / GU::mole);

    auto* gas = new GeoMaterial("TestGas", kGasDensity * GU::g / GU::cm3);
    gas->add(nitrogen, 0.77);
    gas->add(oxygen, 0.23);
    gas->lock();
    auto* iron = new GeoMaterial("TestIron", kIronDensity * GU::g / GU::cm3);
    iron->add(ironElement, 1.0);
    iron->lock();
    auto* copper = new GeoMaterial("TestCopper", kCopperDensity * GU::g / GU::cm3);
    copper->add(copperElement, 1.0);
    copper->lock();

    auto* world = new GeoPhysVol(
        new GeoLogVol("TestWorld", new GeoBox(kWorldHalf, kWorldHalf, kWorldHalf), gas));
    world->add(new GeoTransform(GeoTrf::Translate3D(0, 0, kIronZ)));
    world->add(new GeoPhysVol(
        new GeoLogVol("TestIronBox", new GeoBox(kCubeHalf, kCubeHalf, kCubeHalf), iron)));
    world->add(new GeoTransform(GeoTrf::Translate3D(0, 0, kCopperZ)));
    world->add(new GeoPhysVol(
        new GeoLogVol("TestCopperBox", new GeoBox(kCubeHalf, kCubeHalf, kCubeHalf), copper)));
    return world;
}
