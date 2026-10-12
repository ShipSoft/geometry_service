# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

find_package(Catch2 3 REQUIRED)

include(Catch)

foreach(
    test
    IN
    ITEMS test_geometry_service test_ray_scanner test_geometry_thread
)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(
        ${test}
        PRIVATE SHiPGeometryService Catch2::Catch2WithMain
    )
    catch_discover_tests(${test})
endforeach()

# GeoModelWrite/GeoModelDBManager: the registry tests write temporary .db
# files to load through sharedFromFile.
target_link_libraries(
    test_geometry_thread
    PRIVATE GeoModelIO::GeoModelDBManager GeoModelIO::GeoModelWrite
)
