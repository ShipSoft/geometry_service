# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

project(
    SHiPGeometryService
    VERSION 0.5.0
    DESCRIPTION "SHiP geometry service library (framework-agnostic)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# clang-tidy replays these commands; see .github/workflows/clang-tidy.yml.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build shared libraries by default" ON)

# ----------------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------------

find_package(SHiPGeometry CONFIG REQUIRED)
# 6.30 introduced the VolumeBuilder API used in GeometryService.cpp.
# GeoModelG4 installs no version file, so the floor sits on its siblings,
# which GeoModel releases in lockstep with it.
find_package(GeoModelCore 6.30 CONFIG REQUIRED)
find_package(GeoModelIO 6.30 CONFIG REQUIRED)
find_package(GeoModelG4 CONFIG REQUIRED)
find_package(Geant4 CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Resolve install prefixes from the *_DIR cache variables set by find_package.
# These are embedded in the installed config so find_dependency succeeds
# without downstream consumers needing to set CMAKE_PREFIX_PATH.
cmake_path(GET SHiPGeometry_DIR PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _d)
cmake_path(GET _d PARENT_PATH _geometry_prefix)
unset(_d)

# ----------------------------------------------------------------------------
# Library
# ----------------------------------------------------------------------------

add_library(
    SHiPGeometryService
    src/GeometryService.cpp
    src/G4RayScanner.cpp
    src/GeometryThread.cpp
)

set_target_properties(
    SHiPGeometryService
    PROPERTIES EXPORT_NAME GeometryService
)
add_library(SHiP::GeometryService ALIAS SHiPGeometryService)

# Pre-1.0, a minor release may break the ABI, so the soname carries both
# major and minor.
set_target_properties(
    SHiPGeometryService
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# The file set supplies both the build-tree and the installed include path,
# and installs the headers below.
target_sources(
    SHiPGeometryService
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS include
            FILES
                include/SHiP/GeometryService/G4RayScanner.h
                include/SHiP/GeometryService/GeometryService.h
                include/SHiP/GeometryService/GeometryThread.h
                include/SHiP/GeometryService/IGeometryService.h
                # Forwarders for the pre-0.5.0 paths, removed in 0.6.0.
                include/GeometryService/G4RayScanner.h
                include/GeometryService/GeometryThread.h
                include/GeometryService/IGeometryService.h
                include/GeometryService/SHiPGeometryService.h
)

# CMAKE_CXX_STANDARD above is directory-scope only and not exported;
# without this, the installed target carries no language requirement and
# consumers on an older default standard get header errors instead of a
# clear CMake diagnostic.
target_compile_features(SHiPGeometryService PUBLIC cxx_std_23)

# GeoModelKernel (PVConstLink), Geant4 (G4RayScanner.h holds a G4Navigator
# by value) and std::thread (GeometryThread.h) appear in public headers;
# everything else is implementation detail. Geant4_LIBRARIES expands to
# imported target names (modern Geant4 configs), which the exported target
# records verbatim; the installed config's find_dependency(Geant4)
# re-creates them downstream.
target_link_libraries(
    SHiPGeometryService
    PUBLIC GeoModelCore::GeoModelKernel ${Geant4_LIBRARIES} Threads::Threads
    PRIVATE
        SHiPGeometry::SHiPGeometry
        GeoModelIO::GeoModelDBManager
        GeoModelIO::GeoModelRead
        GeoModel2G4
)

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

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
    FILE_SET HEADERS
)

install(
    EXPORT SHiPGeometryServiceTargets
    FILE SHiPGeometryServiceTargets.cmake
    NAMESPACE SHiP::
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
