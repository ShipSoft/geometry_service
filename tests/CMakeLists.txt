# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

find_package(Catch2 3 REQUIRED)

include(Catch)

# Core tests link the core library only, so a GeoModel or Geant4 symbol
# leaking into its interface fails to link here.
foreach(test IN ITEMS test_geometry_service test_core_headers)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(
        ${test}
        PRIVATE SHiP::GeometryService Catch2::Catch2WithMain
    )
    catch_discover_tests(${test})
endforeach()

add_executable(test_geomodel_interop test_geomodel_interop.cpp)
target_link_libraries(
    test_geomodel_interop
    PRIVATE SHiP::GeometryServiceGeoModel Catch2::Catch2WithMain
)
catch_discover_tests(test_geomodel_interop)

if(BUILD_G4)
    foreach(
        test
        IN
        ITEMS
            test_geant4_geometry
            test_ray_scanner
            test_geometry_thread
            test_store_cleanup
    )
        add_executable(${test} ${test}.cpp)
        target_link_libraries(
            ${test}
            PRIVATE
                SHiP::GeometryServiceG4
                SHiP::GeometryServiceGeoModel
                Catch2::Catch2WithMain
        )
        catch_discover_tests(${test})
    endforeach()

    # GeoModelWrite/GeoModelDBManager: the registry tests write temporary .db
    # files to load through sharedFromFile.
    target_link_libraries(
        test_geometry_thread
        PRIVATE GeoModelIO::GeoModelDBManager GeoModelIO::GeoModelWrite
    )
endif()
