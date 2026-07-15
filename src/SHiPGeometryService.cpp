// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <G4LogicalVolumeStore.hh>

#include <GeoModel2G4/ExtParameterisedVolumeBuilder.h>
#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelRead/ReadGeoModel.h>
#include <SHiPGeometry/SHiPGeometry.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace ship {

// ----------------------------------------------------------------------------
// Factory: build from C++ source
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromSource() {
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    SHiPGeometry::SHiPGeometryBuilder builder;
    svc->m_world = builder.build();
    if (!svc->m_world)
        throw std::runtime_error("SHiPGeometryService::fromSource: geometry build returned null");
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: load from GeoModel .db file
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromFile(const std::string& dbPath) {
    // GMDBManager opens via sqlite3_open, which silently creates a missing
    // file; validate up front to fail with a clear message instead.
    if (!std::filesystem::is_regular_file(dbPath))
        throw std::runtime_error("SHiPGeometryService::fromFile: no such file: " + dbPath);
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    auto db = std::make_shared<GMDBManager>(dbPath);
    GeoModelIO::ReadGeoModel reader(db);
    svc->m_world = reader.buildGeoModel();
    if (!svc->m_world)
        throw std::runtime_error("SHiPGeometryService::fromFile: buildGeoModel returned null");
    return svc;
}

// ----------------------------------------------------------------------------
// Factory: wrap an existing GeoModel tree
// ----------------------------------------------------------------------------

std::unique_ptr<SHiPGeometryService> SHiPGeometryService::fromWorld(PVConstLink world) {
    if (!world)
        throw std::runtime_error("SHiPGeometryService::fromWorld: world is null");
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    svc->m_world = std::move(world);
    return svc;
}

// ----------------------------------------------------------------------------
// IGeometryService implementation
// ----------------------------------------------------------------------------

const GeoVPhysVol* SHiPGeometryService::geoModelWorld() const {
    return m_world.get();
}

G4LogicalVolume* SHiPGeometryService::geant4WorldLogical() {
    std::call_once(m_g4ConversionDone, [this]() {
        ExtParameterisedVolumeBuilder builder("SHiP");
        m_g4WorldLV = builder.Build(m_world);
        if (!m_g4WorldLV)
            throw std::runtime_error("SHiPGeometryService: GeoModel→Geant4 conversion failed");
    });
    return m_g4WorldLV;
}

G4LogicalVolume* SHiPGeometryService::getLogicalVolume(const std::string& name) const {
    return G4LogicalVolumeStore::GetInstance()->GetVolume(name, /*verbose=*/false);
}

}  // namespace ship
