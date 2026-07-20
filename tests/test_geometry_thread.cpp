// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/G4RayScanner.h"
#include "GeometryService/GeometryThread.h"
#include "GeometryService/SHiPGeometryService.h"
#include "test_world.h"

#include <G4GeometryManager.hh>
#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4Material.hh>
#include <G4PVPlacement.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4Region.hh>
#include <G4RegionStore.hh>
#include <G4SolidStore.hh>
#include <G4Threading.hh>
#include <G4WorkerThread.hh>

#include <thread>

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelWrite/WriteGeoModel.h>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

using Catch::Matchers::WithinAbs;
using ship::G4RayScanner;
using ship::SHiPGeometryService;

// ----------------------------------------------------------------------------
// GeometryThread
// ----------------------------------------------------------------------------

TEST_CASE("GeometryThreadTest.RunsTasksOnOneDedicatedThread", "[geometry_thread]") {
    auto& gt = ship::geometry_thread();
    auto first = gt.run([] { return std::this_thread::get_id(); });
    auto second = gt.run([] { return std::this_thread::get_id(); });
    CHECK(first == second);
    CHECK(first != std::this_thread::get_id());
}

TEST_CASE("GeometryThreadTest.NestedRunExecutesInline", "[geometry_thread]") {
    auto& gt = ship::geometry_thread();
    auto ids = gt.run([&gt] {
        return std::pair{std::this_thread::get_id(),
                         gt.run([] { return std::this_thread::get_id(); })};
    });
    CHECK(ids.first == ids.second);
}

TEST_CASE("GeometryThreadTest.ExceptionsPropagateToTheCaller", "[geometry_thread]") {
    CHECK_THROWS_AS(ship::geometry_thread().run([]() -> int { throw std::runtime_error("boom"); }),
                    std::runtime_error);
    // The thread survives a throwing task.
    CHECK(ship::geometry_thread().run([] { return 42; }) == 42);
}

TEST_CASE("GeometryThreadTest.ConcurrentCallersSerialise", "[geometry_thread]") {
    auto& gt = ship::geometry_thread();
    std::atomic<int> active{0};
    std::atomic<bool> overlapped{false};
    auto task = [&] {
        if (++active > 1)
            overlapped = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        --active;
    };
    std::thread a{[&] {
        for (int i = 0; i < 10; ++i)
            gt.run(task);
    }};
    std::thread b{[&] {
        for (int i = 0; i < 10; ++i)
            gt.run(task);
    }};
    a.join();
    b.join();
    CHECK_FALSE(overlapped);
}

// ----------------------------------------------------------------------------
// SHiPGeometryService::sharedFromFile
// ----------------------------------------------------------------------------

namespace {

// Writes the shared test world to a fresh GeoModel .db at `path`.
void writeTestDb(const std::filesystem::path& path) {
    std::filesystem::remove(path);
    GMDBManager db{path.string()};
    REQUIRE(db.checkIsDBOpen());
    GeoModelIO::WriteGeoModel dumper{db};
    buildTestWorld()->exec(&dumper);
    dumper.saveToDB();
}

std::filesystem::path testDir() {
    auto dir = std::filesystem::temp_directory_path() /
               ("shipgeomsvc_registry_" + std::to_string(getpid()));
    std::filesystem::create_directories(dir);
    return dir;
}

}  // namespace

TEST_CASE("SharedFromFileTest.SameCanonicalPathYieldsSameInstance", "[shared_registry]") {
    auto dir = testDir();
    writeTestDb(dir / "world.db");
    auto first = SHiPGeometryService::sharedFromFile((dir / "world.db").string());
    auto second = SHiPGeometryService::sharedFromFile((dir / "world.db").string());
    CHECK(first == second);
    // A non-canonical spelling of the same file hits the same entry.
    auto dotted = SHiPGeometryService::sharedFromFile((dir / "." / "world.db").string());
    CHECK(dotted == first);
    std::filesystem::remove_all(dir);
}

TEST_CASE("SharedFromFileTest.DifferentFilesYieldDistinctInstances", "[shared_registry]") {
    auto dir = testDir();
    writeTestDb(dir / "one.db");
    writeTestDb(dir / "two.db");
    auto one = SHiPGeometryService::sharedFromFile((dir / "one.db").string());
    auto two = SHiPGeometryService::sharedFromFile((dir / "two.db").string());
    CHECK(one != two);
    std::filesystem::remove_all(dir);
}

TEST_CASE("SharedFromFileTest.ExpiredEntryIsReloaded", "[shared_registry]") {
    auto dir = testDir();
    writeTestDb(dir / "world.db");
    auto path = (dir / "world.db").string();
    auto first = SHiPGeometryService::sharedFromFile(path);
    auto* raw = first.get();
    first.reset();  // last owner: the registry's weak_ptr expires
    auto reloaded = SHiPGeometryService::sharedFromFile(path);
    CHECK(reloaded.get() != raw);
    std::filesystem::remove_all(dir);
}

TEST_CASE("SharedFromFileTest.ReloadAfterConversionIsSafe", "[shared_registry]") {
    auto dir = testDir();
    writeTestDb(dir / "world.db");
    auto path = (dir / "world.db").string();
    auto& gt = ship::geometry_thread();

    auto first = SHiPGeometryService::sharedFromFile(path);
    REQUIRE(gt.run([&] { return first->geant4WorldLogical(); }) != nullptr);
    first.reset();  // converted instance expires; its GeoModel tree is retained

    auto reloaded = SHiPGeometryService::sharedFromFile(path);
    CHECK(gt.run([&] { return reloaded->geant4WorldLogical(); }) != nullptr);

    // Clean the stores on the geometry thread (see the final test for why
    // leaving them to static teardown on the main thread would segfault).
    gt.run([&] {
        G4GeometryManager::GetInstance()->OpenGeometry();
        G4PhysicalVolumeStore::Clean();
        G4LogicalVolumeStore::Clean();
        G4SolidStore::Clean();
    });
    std::filesystem::remove_all(dir);
}

TEST_CASE("SharedFromFileTest.MissingFileThrows", "[shared_registry]") {
    CHECK_THROWS_AS(SHiPGeometryService::sharedFromFile("/nonexistent/path.db"),
                    std::runtime_error);
}

// ----------------------------------------------------------------------------
// Two Geant4 users, one creator thread — the aegir-genie issue #11 scenario
// ----------------------------------------------------------------------------

// A GENIE-style user (convert + ray-scan) and a run-manager-style user
// (world placement, region, full geometry close) share the geometry through
// ship::geometry_thread(); a worker-style thread replicates the split-class
// arrays and reads. Before the shared thread existed, the second user's
// touch of the converted volumes segfaulted (Geant4 bug #2747). Runs last
// in this binary: it populates the Geant4 stores.
TEST_CASE("GeometryThreadTest.TwoGeant4UsersShareOneCreatorThread", "[geometry_thread]") {
    auto& gt = ship::geometry_thread();

    // GENIE-style user: convert and scan on the geometry thread.
    auto svc = SHiPGeometryService::fromWorld(buildTestWorld());
    auto* worldLV = gt.run([&] { return svc->geant4WorldLogical(); });
    REQUIRE(worldLV != nullptr);
    auto scanner = gt.run([&] { return std::make_unique<G4RayScanner>(worldLV); });
    auto checkScan = [&] {
        auto [entry, segments] = gt.run([&] { return scanner->scan({0, 0, -1500}, {0, 0, 1}); });
        CHECK_THAT(entry, WithinAbs(500., 1e-6));
        REQUIRE(segments.size() == 5U);
        CHECK(segments[1].material->GetName() == "TestCopper");
        CHECK(segments[3].material->GetName() == "TestIron");
    };
    checkScan();

    // Run-manager-style user, on the same thread: what geant4_module's
    // master init does — reopen, place the shared world, create a region,
    // close (voxelise) the full geometry including the scanner's tree.
    gt.run([&] {
        G4GeometryManager::GetInstance()->OpenGeometry();
        new G4PVPlacement(nullptr, {}, worldLV, worldLV->GetName(), nullptr, false, 0);
        auto* region = new G4Region("TestWorldRegion");
        region->AddRootLogicalVolume(worldLV);
        G4GeometryManager::GetInstance()->CloseGeometry(true, false);
    });

    // Worker-style reader on a fresh thread: replicate the split-class
    // arrays and read converted volumes, as geant4_module's workers do.
    {
        std::exception_ptr error;
        bool readOk = false;
        std::thread worker{[&] {
            try {
                G4Threading::G4SetThreadId(1);
                G4WorkerThread::BuildGeometryAndPhysicsVector();
                auto* iron = G4LogicalVolumeStore::GetInstance()->GetVolume("TestIronBox", false);
                readOk = iron != nullptr && iron->GetSolid() != nullptr &&
                         iron->GetMaterial()->GetName() == "TestIron";
            } catch (...) {
                error = std::current_exception();
            }
        }};
        worker.join();
        if (error)
            std::rethrow_exception(error);
        CHECK(readOk);
    }

    // The scanner keeps working after the run-manager-style close.
    checkScan();

    // Clean the stores on the geometry thread: their static destructors run
    // on the main thread at exit, which has no split-class arrays for these
    // volumes and would segfault deleting them (cf. aegir issue #68).
    gt.run([&] {
        scanner.reset();
        G4GeometryManager::GetInstance()->OpenGeometry();
        G4RegionStore::Clean();
        G4PhysicalVolumeStore::Clean();
        G4LogicalVolumeStore::Clean();
        G4SolidStore::Clean();
    });
}
