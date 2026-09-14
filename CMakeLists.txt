cmake_minimum_required(VERSION 3.20)

project(PingoLegends CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all source files under src/.
# CONFIGURE_DEPENDS makes CMake look for new files when building,
# so new .cpp/.h files do not have to be added by hand.
file(GLOB_RECURSE PINGO_SOURCES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.h
)

add_executable(PingoLegends ${PINGO_SOURCES})

# Includes start at src/, so "Engine/Assets.h" is written the same everywhere
target_include_directories(PingoLegends PRIVATE ${PROJECT_SOURCE_DIR}/src)

find_package(raylib REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(PingoLegends
        PRIVATE
        raylib
        nlohmann_json::nlohmann_json
)

if (NOT MSVC)
    target_compile_options(PingoLegends PRIVATE -Wall -Wextra)
endif ()

# Build mode of the game, independent of whether debug symbols are built:
#   Release      the finished game, without tools
#   Debugging    playable, plus FPS display, hitboxes and scene keys
#   Development  not playable: the world stands still, free camera, tile editor
# In CLion the mode is chosen via the CMake profile, see .idea/cmake.xml.
set(PINGO_BUILD_MODES Release Debugging Development)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PINGO_DEFAULT_BUILD_MODE Debugging)
else ()
    set(PINGO_DEFAULT_BUILD_MODE Release)
endif ()

set(PINGO_BUILD_MODE ${PINGO_DEFAULT_BUILD_MODE} CACHE STRING "Release, Debugging oder Development")
set_property(CACHE PINGO_BUILD_MODE PROPERTY STRINGS ${PINGO_BUILD_MODES})

if (NOT PINGO_BUILD_MODE IN_LIST PINGO_BUILD_MODES)
    message(FATAL_ERROR "PINGO_BUILD_MODE=${PINGO_BUILD_MODE} gibt es nicht. Moeglich: ${PINGO_BUILD_MODES}")
endif ()

message(STATUS "PingoLegends Build-Modus: ${PINGO_BUILD_MODE}")

if (PINGO_BUILD_MODE STREQUAL "Debugging")
    target_compile_definitions(PingoLegends PRIVATE PINGO_MODE_DEBUGGING)
elseif (PINGO_BUILD_MODE STREQUAL "Development")
    # The editors also save to the asset folder of the sources, because
    # the copy in the build directory is overwritten on every build
    target_compile_definitions(PingoLegends PRIVATE
            PINGO_MODE_DEVELOPMENT
            "PINGO_ASSET_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}/assets\""
    )
endif ()

# Copy assets into the build directory.
# Not as POST_BUILD, because that only runs when code was recompiled as well:
# a tweaked JSON would otherwise never reach the game. PingoLegends depends
# on this target instead, so the IDE builds it when starting the game.
#
# Deliberately without comparing timestamps: downloaded assets often come
# with an old modification date and would then never be copied.
add_custom_target(CopyAssets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:PingoLegends>/assets
        COMMENT "Kopiere Assets"
)

add_dependencies(PingoLegends CopyAssets)
