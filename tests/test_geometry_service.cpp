// SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/GeometryService.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

using SHiP::GeometryService;

TEST_CASE("GeometryServiceTest.FromFileMissingFileThrows", "[geometry_service]") {
    CHECK_THROWS_AS(GeometryService::fromFile("/nonexistent/path.db"), std::runtime_error);
}

TEST_CASE("GeometryServiceTest.FromFileCorruptFileThrows", "[geometry_service]") {
    const auto path = std::filesystem::temp_directory_path() /
                      ("shipgeomsvc_corrupt_" + std::to_string(getpid()) + ".db");
    {
        std::ofstream out{path};
        out << "not a sqlite database";
    }
    CHECK_THROWS_AS(GeometryService::fromFile(path), std::runtime_error);
    std::filesystem::remove(path);
}

namespace {

// A fresh directory holding share/geometry/<name>, with SHIPGEOMETRY_ROOT
// pointing at it for the lifetime of the object.
class InstalledGeometry {
   public:
    explicit InstalledGeometry(const std::string& name)
        : m_root{std::filesystem::temp_directory_path() /
                 ("shipgeomsvc_root_" + std::to_string(getpid()))} {
        std::filesystem::create_directories(m_root / "share" / "geometry");
        std::ofstream{file(name)} << "placeholder";
        // NOLINTNEXTLINE(concurrency-mt-unsafe): single-threaded test setup
        setenv("SHIPGEOMETRY_ROOT", m_root.c_str(), 1);
    }
    ~InstalledGeometry() {
        // NOLINTNEXTLINE(concurrency-mt-unsafe): single-threaded test teardown
        unsetenv("SHIPGEOMETRY_ROOT");
        std::filesystem::remove_all(m_root);
    }
    InstalledGeometry(const InstalledGeometry&) = delete;
    InstalledGeometry& operator=(const InstalledGeometry&) = delete;
    InstalledGeometry(InstalledGeometry&&) = delete;
    InstalledGeometry& operator=(InstalledGeometry&&) = delete;

    [[nodiscard]] std::filesystem::path file(const std::string& name) const {
        return m_root / "share" / "geometry" / name;
    }

   private:
    std::filesystem::path m_root;
};

}  // namespace

TEST_CASE("ResolveGeometryDbPath.ExistingPathIsReturnedAsGiven", "[geometry_service]") {
    const InstalledGeometry installed{"installed.db"};
    const auto path = installed.file("installed.db");
    CHECK(SHiP::resolveGeometryDbPath(path) == path);
}

TEST_CASE("ResolveGeometryDbPath.BareNameIsFoundInTheInstalledGeometry", "[geometry_service]") {
    const InstalledGeometry installed{"installed.db"};
    CHECK(SHiP::resolveGeometryDbPath("installed.db") == installed.file("installed.db"));
}

TEST_CASE("ResolveGeometryDbPath.MissingBareNameThrows", "[geometry_service]") {
    const InstalledGeometry installed{"installed.db"};
    CHECK_THROWS_AS(SHiP::resolveGeometryDbPath("other.db"), std::runtime_error);
}

TEST_CASE("ResolveGeometryDbPath.MissingPathWithDirectoryDoesNotFallBack", "[geometry_service]") {
    // The file name exists in the installed geometry, but the path names a
    // different, missing file.
    const InstalledGeometry installed{"installed.db"};
    CHECK_THROWS_AS(SHiP::resolveGeometryDbPath("mistyped/installed.db"), std::runtime_error);
}

TEST_CASE("ResolveGeometryDbPath.EmptyPathThrows", "[geometry_service]") {
    // Appending an empty path would name the installed geometry directory.
    const InstalledGeometry installed{"installed.db"};
    CHECK_THROWS_AS(SHiP::resolveGeometryDbPath(""), std::runtime_error);
}
