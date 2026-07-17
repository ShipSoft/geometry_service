# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
    FIND_PACKAGE_ARGS NAMES Catch2
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(Catch)

add_executable(test_geometry_service test_geometry_service.cpp)

target_link_libraries(
    test_geometry_service
    PRIVATE SHiPGeometryService Catch2::Catch2WithMain
)

catch_discover_tests(test_geometry_service)

add_executable(test_ray_scanner test_ray_scanner.cpp)

target_link_libraries(
    test_ray_scanner
    PRIVATE SHiPGeometryService Catch2::Catch2WithMain
)

catch_discover_tests(test_ray_scanner)

add_executable(test_geometry_thread test_geometry_thread.cpp)

# GeoModelWrite/GeoModelDBManager: the registry tests write temporary .db
# files to load through sharedFromFile.
target_link_libraries(
    test_geometry_thread
    PRIVATE
        SHiPGeometryService
        Catch2::Catch2WithMain
        GeoModelIO::GeoModelDBManager
        GeoModelIO::GeoModelWrite
)

catch_discover_tests(test_geometry_thread)
