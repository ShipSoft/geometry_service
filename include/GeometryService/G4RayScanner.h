// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <G4Navigator.hh>
#include <G4ThreeVector.hh>

#include <vector>

class G4LogicalVolume;
class G4Material;
class G4VPhysicalVolume;

namespace ship {

/// One material segment along a straight ray, in Geant4 native units (mm).
struct RaySegment {
    const G4Material* material;  ///< borrowed from the Geant4 geometry
    double length;               ///< mm
};

/// Result of a ray scan: where the ray entered the world and what it crossed.
struct RayScan {
    double entry{0.};                  ///< distance (mm) from the ray origin to
                                       ///< the start of the first segment
    std::vector<RaySegment> segments;  ///< ordered material segments (mm)
};

/// Straight-line material scanner over a Geant4 geometry.
///
/// Uses a private G4Navigator; no G4RunManager is required. The constructor
/// wraps `top` in its own world placement at the origin, so scan()
/// coordinates are expressed in `top`'s local frame — pass the world logical
/// volume to scan in global coordinates, or any daughter logical volume to
/// scan its subtree in that volume's frame.
///
/// NOT thread-safe: with a multithreaded Geant4 build, logical/physical
/// volumes carry thread-local state, so construct and use the scanner on the
/// thread that built the Geant4 volumes.
///
/// G4GeometryManager's closed state is a single process-wide flag, so at
/// most one scanner may be alive while the geometry is closed: constructing
/// a scanner while another one (or a run manager) holds the geometry closed
/// throws instead of silently scanning without voxelisation.
class G4RayScanner {
   public:
    /// Wraps `top` in a private world placement and closes (voxelises) that
    /// tree via G4GeometryManager. Throws std::invalid_argument on null and
    /// std::logic_error if the Geant4 geometry is already closed.
    explicit G4RayScanner(G4LogicalVolume* top);

    /// Re-opens the scanned tree and removes the private placement (skipped
    /// if another Geant4 user already cleaned the volume stores).
    ~G4RayScanner();

    G4RayScanner(const G4RayScanner&) = delete;
    G4RayScanner& operator=(const G4RayScanner&) = delete;
    G4RayScanner(G4RayScanner&&) = delete;
    G4RayScanner& operator=(G4RayScanner&&) = delete;

    /// Ordered material segments from `origin` (mm) along `direction`
    /// (normalised internally) up to the world boundary. A ray starting
    /// outside the world is first advanced to its entry point (recorded as
    /// `entry`); a ray that misses the world returns no segments. Adjacent
    /// segments in the same material are merged. Throws std::runtime_error
    /// if navigation fails to terminate (step-limit safeguard).
    [[nodiscard]] RayScan scan(const G4ThreeVector& origin, const G4ThreeVector& direction) const;

   private:
    G4VPhysicalVolume* m_world{nullptr};
    mutable G4Navigator m_navigator;
};

}  // namespace ship
