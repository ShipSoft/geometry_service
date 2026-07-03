# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    FIND_PACKAGE_ARGS NAMES GTest
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(test_geometry_service test_geometry_service.cpp)

target_link_libraries(
    test_geometry_service
    PRIVATE SHiPGeometryService GTest::gtest_main
)

gtest_discover_tests(test_geometry_service DISCOVERY_MODE PRE_TEST)
