# SPDX-FileCopyrightText: 2025 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

find_package(ROOT QUIET COMPONENTS Hist Graf Gpad)
if(NOT ROOT_FOUND)
    message(STATUS "ROOT not found — skipping plot_field_map")
    return()
endif()

include(${ROOT_USE_FILE})

add_executable(plot_field_map plot_field_map.cpp)
target_link_libraries(
    plot_field_map
    PRIVATE SHiPGeometryService ROOT::Hist ROOT::Graf ROOT::Gpad ROOT::Core
)
