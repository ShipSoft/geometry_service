// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/SHiPMagneticFieldService.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH2F.h>

#include <iostream>
#include <mp-units/systems/si.h>
#include <string>

int main(int argc, char* argv[]) {
    std::string outFile = "field_map.root";
    if (argc > 1) {
        outFile = argv[1];
    }

    // Compute stem for PDF filenames (strip .root suffix if present)
    std::string stem = outFile;
    if (stem.size() > 5 && stem.substr(stem.size() - 5) == ".root") {
        stem = stem.substr(0, stem.size() - 5);
    }

    auto svc = ship::SHiPMagneticFieldService::withDefaultRegions();
    using namespace mp_units::si::unit_symbols;

    // ── 1D: By vs z at (x=0, y=0) ─────────────────────────────────────────
    constexpr int nZ1D = 1000;
    TGraph gBy_z(nZ1D);
    gBy_z.SetName("gBy_z");
    gBy_z.SetTitle("B_{y} along beam axis;z [m];B_{y} [T]");
    for (int i = 0; i < nZ1D; ++i) {
        double z_mm = i * 100.0;  // 0 to 99 900 mm in 100 mm steps
        auto f = svc->getField(0.0 * mm, 0.0 * mm, z_mm * mm);
        gBy_z.SetPoint(i, z_mm / 1000.0, f[1].numerical_value_in(T));
    }

    // ── 2D: By in xz-plane at y=0 ─────────────────────────────────────────
    // z on x-axis (200 bins, 0–100 m), x on y-axis (70 bins, −3.5–+3.5 m)
    constexpr int nZ_xz = 200, nX_xz = 70;
    auto* hBy_xz = new TH2F("hBy_xz", "B_{y} in xz-plane (y=0);z [m];x [m];B_{y} [T]", nZ_xz, 0.0,
                            100.0, nX_xz, -3.5, 3.5);
    for (int iz = 0; iz < nZ_xz; ++iz) {
        double z_mm = (iz + 0.5) * (100000.0 / nZ_xz);
        for (int ix = 0; ix < nX_xz; ++ix) {
            double x_mm = -3500.0 + (ix + 0.5) * (7000.0 / nX_xz);
            auto f = svc->getField(x_mm * mm, 0.0 * mm, z_mm * mm);
            hBy_xz->SetBinContent(iz + 1, ix + 1, f[1].numerical_value_in(T));
        }
    }

    // ── 2D: By in yz-plane at x=0 ─────────────────────────────────────────
    // z on x-axis (200 bins, 0–100 m), y on y-axis (90 bins, −4.5–+4.5 m)
    constexpr int nZ_yz = 200, nY_yz = 90;
    auto* hBy_yz = new TH2F("hBy_yz", "B_{y} in yz-plane (x=0);z [m];y [m];B_{y} [T]", nZ_yz, 0.0,
                            100.0, nY_yz, -4.5, 4.5);
    for (int iz = 0; iz < nZ_yz; ++iz) {
        double z_mm = (iz + 0.5) * (100000.0 / nZ_yz);
        for (int iy = 0; iy < nY_yz; ++iy) {
            double y_mm = -4500.0 + (iy + 0.5) * (9000.0 / nY_yz);
            auto f = svc->getField(0.0 * mm, y_mm * mm, z_mm * mm);
            hBy_yz->SetBinContent(iz + 1, iy + 1, f[1].numerical_value_in(T));
        }
    }

    // ── 2D: By in xy-plane at MuonShield centre ────────────────────────────
    constexpr double z_ms_mm = 16763.3;
    constexpr int nX_ms = 36, nY_ms = 34;
    auto* hBy_xy_ms =
        new TH2F("hBy_xy_ms", "B_{y} at MuonShield centre (z=16.76 m);x [m];y [m];B_{y} [T]", nX_ms,
                 -1.81, 1.81, nY_ms, -1.70, 1.70);
    for (int ix = 0; ix < nX_ms; ++ix) {
        double x_mm = -1810.0 + (ix + 0.5) * (3620.0 / nX_ms);
        for (int iy = 0; iy < nY_ms; ++iy) {
            double y_mm = -1700.0 + (iy + 0.5) * (3400.0 / nY_ms);
            auto f = svc->getField(x_mm * mm, y_mm * mm, z_ms_mm * mm);
            hBy_xy_ms->SetBinContent(ix + 1, iy + 1, f[1].numerical_value_in(T));
        }
    }

    // ── 2D: By in xy-plane at Spectrometer centre ──────────────────────────
    constexpr double z_spec_mm = 89570.0;
    constexpr int nX_spec = 65, nY_spec = 86;
    auto* hBy_xy_spec =
        new TH2F("hBy_xy_spec", "B_{y} at Spectrometer centre (z=89.57 m);x [m];y [m];B_{y} [T]",
                 nX_spec, -3.25, 3.25, nY_spec, -4.30, 4.30);
    for (int ix = 0; ix < nX_spec; ++ix) {
        double x_mm = -3250.0 + (ix + 0.5) * (6500.0 / nX_spec);
        for (int iy = 0; iy < nY_spec; ++iy) {
            double y_mm = -4300.0 + (iy + 0.5) * (8600.0 / nY_spec);
            auto f = svc->getField(x_mm * mm, y_mm * mm, z_spec_mm * mm);
            hBy_xy_spec->SetBinContent(ix + 1, iy + 1, f[1].numerical_value_in(T));
        }
    }

    // ── Write ROOT file ────────────────────────────────────────────────────
    {
        TFile out(outFile.c_str(), "RECREATE");
        gBy_z.Write();
        hBy_xz->Write();
        hBy_yz->Write();
        hBy_xy_ms->Write();
        hBy_xy_spec->Write();
        out.Close();
    }
    std::cout << "Wrote " << outFile << "  (5 objects)\n";

    // ── PDF export ─────────────────────────────────────────────────────────
    {
        TCanvas c("c_by_z", "", 800, 600);
        c.SetGrid();
        gBy_z.Draw("AL");
        std::string pdf = stem + "_by_z.pdf";
        c.SaveAs(pdf.c_str());
        std::cout << "Saved " << pdf << "\n";
    }
    {
        TCanvas c("c_by_xz", "", 800, 600);
        hBy_xz->Draw("COLZ");
        std::string pdf = stem + "_by_xz.pdf";
        c.SaveAs(pdf.c_str());
        std::cout << "Saved " << pdf << "\n";
    }
    {
        TCanvas c("c_by_yz", "", 800, 600);
        hBy_yz->Draw("COLZ");
        std::string pdf = stem + "_by_yz.pdf";
        c.SaveAs(pdf.c_str());
        std::cout << "Saved " << pdf << "\n";
    }
    {
        TCanvas c("c_by_xy_ms", "", 800, 600);
        hBy_xy_ms->Draw("COLZ");
        std::string pdf = stem + "_by_xy_ms.pdf";
        c.SaveAs(pdf.c_str());
        std::cout << "Saved " << pdf << "\n";
    }
    {
        TCanvas c("c_by_xy_spec", "", 800, 600);
        hBy_xy_spec->Draw("COLZ");
        std::string pdf = stem + "_by_xy_spec.pdf";
        c.SaveAs(pdf.c_str());
        std::cout << "Saved " << pdf << "\n";
    }

    return 0;
}
