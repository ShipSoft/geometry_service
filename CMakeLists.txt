# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)

project(
    SHiPGeometryService
    VERSION 0.1.0
    DESCRIPTION "SHiP geometry service library (framework-agnostic)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build shared libraries by default" ON)

# ----------------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------------

find_package(SHiPGeometry CONFIG REQUIRED)
find_package(GeoModelCore CONFIG REQUIRED)
find_package(GeoModelIO CONFIG REQUIRED)
find_package(GeoModelG4 CONFIG REQUIRED)
find_package(Geant4 REQUIRED)
find_package(Microsoft.GSL REQUIRED) # mp-units depends on this but doesn't find it automatically
find_package(mp-units CONFIG REQUIRED)

# Resolve install prefixes from the *_DIR cache variables set by find_package.
# These are embedded in the installed config so find_dependency succeeds
# without downstream consumers needing to set CMAKE_PREFIX_PATH.
cmake_path(GET Microsoft.GSL_DIR PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _ms_gsl_prefix)

cmake_path(GET mp-units_DIR PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _mp_units_prefix)

cmake_path(GET SHiPGeometry_DIR PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _geometry_prefix)
unset(_d)

# ----------------------------------------------------------------------------
# Library
# ----------------------------------------------------------------------------

add_library(
    SHiPGeometryService
    src/SHiPGeometryService.cpp
    src/SHiPMagneticFieldService.cpp
)

target_include_directories(
    SHiPGeometryService
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE ${Geant4_INCLUDE_DIRS}
)

target_link_libraries(
    SHiPGeometryService
    PUBLIC
        SHiPGeometry::SHiPGeometry
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelDBManager
        GeoModelIO::GeoModelRead
        GeoModel2G4
        mp-units::mp-units
    PRIVATE ${Geant4_LIBRARIES}
)

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------

enable_testing()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
add_subdirectory(apps)

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS SHiPGeometryService
    EXPORT SHiPGeometryServiceTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
    EXPORT SHiPGeometryServiceTargets
    FILE SHiPGeometryServiceTargets.cmake
    NAMESPACE SHiPGeometryService::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometryService
)

configure_package_config_file(
    cmake/SHiPGeometryServiceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryServiceConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometryService
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryServiceConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryServiceConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryServiceConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometryService
)
