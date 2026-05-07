// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPGeometryService.h"

#include <SHiPGeometry/SHiPGeometry.h>

#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelRead/ReadGeoModel.h>
#include <GeoModel2G4/ExtParameterisedVolumeBuilder.h>

#include <G4LogicalVolumeStore.hh>

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
    auto svc = std::unique_ptr<SHiPGeometryService>(new SHiPGeometryService());
    auto db  = std::make_shared<GMDBManager>(dbPath);
    GeoModelIO::ReadGeoModel reader(db);
    svc->m_world = reader.buildGeoModel();
    if (!svc->m_world)
        throw std::runtime_error("SHiPGeometryService::fromFile: buildGeoModel returned null");
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
            throw std::runtime_error(
                "SHiPGeometryService: GeoModel→Geant4 conversion failed");
    });
    return m_g4WorldLV;
}

G4LogicalVolume* SHiPGeometryService::getLogicalVolume(const std::string& name) const {
    auto* store = G4LogicalVolumeStore::GetInstance();
    for (auto* lv : *store) {
        if (lv->GetName() == name)
            return lv;
    }
    return nullptr;
}

}  // namespace ship
