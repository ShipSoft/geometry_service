// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SHiP/GeometryService/GeometryService.h"

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class G4LogicalVolume;

namespace SHiP {

/// The Geant4 view of a GeometryService: the GeoModel tree converted with
/// GeoModel2G4. Part of SHiP::GeometryServiceG4.
///
/// Geant4 volumes register in process-wide stores, so converting the same
/// geometry twice would leave two trees there. of() therefore hands out one
/// instance per service. Each instance keeps its service alive; hold it for
/// as long as the converted volumes are in use.
///
/// Conversion and navigation must run on SHiP::geometry_thread() (see
/// GeometryThread.h) whenever more than one thread touches Geant4 geometry:
/// Geant4 permits only one geometry-creating thread per process.
class Geant4Geometry final {
   public:
    /// The process-wide Geant4 view of `service`; created on first request
    /// and shared until the last holder releases it. Does not convert yet.
    /// Throws std::invalid_argument on a null service.
    [[nodiscard]] static std::shared_ptr<Geant4Geometry> of(
        std::shared_ptr<const GeometryService> service);

    ~Geant4Geometry();

    Geant4Geometry(const Geant4Geometry&) = delete;
    Geant4Geometry& operator=(const Geant4Geometry&) = delete;
    Geant4Geometry(Geant4Geometry&&) = delete;
    Geant4Geometry& operator=(Geant4Geometry&&) = delete;

    /// The Geant4 world logical volume. Converts on first call. Throws if
    /// the conversion fails, on this and every later call, so it never
    /// yields a null volume. When returning it from GeometryThread::run,
    /// give the lambda a reference return type (`[&]() -> auto& { ... }`).
    [[nodiscard]] G4LogicalVolume& worldLogical();

    /// Look up a G4LogicalVolume by name within this converted tree
    /// (depth-first). Returns nullptr before worldLogical() has been called
    /// or when no such volume exists; rethrows a stored conversion failure
    /// instead of masking it as nullptr.
    [[nodiscard]] G4LogicalVolume* logicalVolume(std::string_view name) const;

    /// The service this view converts.
    [[nodiscard]] const GeometryService& service() const noexcept { return *m_service; }

   private:
    explicit Geant4Geometry(std::shared_ptr<const GeometryService> service);

    /// Transparent, so logicalVolume() looks a string_view up without
    /// building a std::string.
    struct NameHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view name) const noexcept {
            return std::hash<std::string_view>{}(name);
        }
    };

    std::shared_ptr<const GeometryService> m_service;
    std::once_flag m_conversionDone;
    G4LogicalVolume* m_worldLV{nullptr};   ///< written once via call_once
    std::exception_ptr m_conversionError;  ///< sticky conversion failure
    /// First converted volume of each name, depth-first; filled with
    /// m_worldLV, so later renames are not seen.
    std::unordered_map<std::string, G4LogicalVolume*, NameHash, std::equal_to<>> m_lvByName;
};

}  // namespace SHiP
