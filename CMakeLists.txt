cmake_minimum_required(VERSION 3.20)

project(PingoEngine CXX)

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

add_executable(PingoEngine ${PINGO_SOURCES})

# On macOS some parts of the engine talk to Cocoa directly, e.g. the
# ClickMonitor. They live in .mm files: Objective-C++ with automatic
# reference counting.
if (APPLE)
    enable_language(OBJCXX)

    file(GLOB_RECURSE PINGO_APPLE_SOURCES CONFIGURE_DEPENDS
            ${PROJECT_SOURCE_DIR}/src/*.mm
    )

    target_sources(PingoEngine PRIVATE ${PINGO_APPLE_SOURCES})
    set_source_files_properties(${PINGO_APPLE_SOURCES} PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    target_link_libraries(PingoEngine PRIVATE "-framework AppKit")
endif ()

# Includes start at src/, so "Engine/Assets.h" is written the same everywhere
target_include_directories(PingoEngine PRIVATE ${PROJECT_SOURCE_DIR}/src)

find_package(raylib REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(PingoEngine
        PRIVATE
        raylib
        nlohmann_json::nlohmann_json
)

# The screenshot harness of tools/screenshots: a build that drives the game with
# simulated keys and mouse clicks and saves screenshots, see run.sh there. Off
# by default, it never belongs into a build that is played.
option(PINGO_SCREENSHOT_HARNESS "Build the screenshot harness into the game" OFF)

if (PINGO_SCREENSHOT_HARNESS)
    target_sources(PingoEngine PRIVATE ${PROJECT_SOURCE_DIR}/tools/screenshots/ScreenshotHarness.cpp)
    target_compile_definitions(PingoEngine PRIVATE PINGO_SCREENSHOT_HARNESS)

    # The harness feeds the GLFW callbacks of raylib. The Homebrew dylib hides
    # GLFW, its static library does not, so this build links that one instead.
    get_target_property(PINGO_RAYLIB_INCLUDE raylib INTERFACE_INCLUDE_DIRECTORIES)
    find_library(PINGO_RAYLIB_STATIC NAMES libraylib.a HINTS /opt/homebrew/lib /usr/local/lib REQUIRED)

    get_target_property(PINGO_LINKED PingoEngine LINK_LIBRARIES)
    list(REMOVE_ITEM PINGO_LINKED raylib)
    set_target_properties(PingoEngine PROPERTIES LINK_LIBRARIES "${PINGO_LINKED}")

    target_link_libraries(PingoEngine PRIVATE
            ${PINGO_RAYLIB_STATIC}
            "-framework Cocoa" "-framework IOKit" "-framework CoreVideo" "-framework CoreAudio"
            "-framework AudioToolbox" "-framework OpenGL"
    )
endif ()

if (NOT MSVC)
    target_compile_options(PingoEngine PRIVATE -Wall -Wextra)
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

message(STATUS "PingoEngine Build-Modus: ${PINGO_BUILD_MODE}")

if (PINGO_BUILD_MODE STREQUAL "Debugging")
    target_compile_definitions(PingoEngine PRIVATE PINGO_MODE_DEBUGGING)
elseif (PINGO_BUILD_MODE STREQUAL "Development")
    # The editors also save to the asset folder of the sources, because
    # the copy in the build directory is overwritten on every build.
    # Scripts are code and only exist in the sources, see ScriptFiles.
    set(PINGO_ASSET_SOURCE_DIR_VALUE ${PROJECT_SOURCE_DIR}/assets)
    set(PINGO_SCRIPT_SOURCE_DIR_VALUE ${PROJECT_SOURCE_DIR}/scripts)

    # The screenshot build must not write into the sources. Relative to the
    # working directory the copies land in the folder of the run.
    if (PINGO_SCREENSHOT_HARNESS)
        set(PINGO_ASSET_SOURCE_DIR_VALUE assets)
        set(PINGO_SCRIPT_SOURCE_DIR_VALUE scripts)
    endif ()

    target_compile_definitions(PingoEngine PRIVATE
            PINGO_MODE_DEVELOPMENT
            "PINGO_ASSET_SOURCE_DIR=\"${PINGO_ASSET_SOURCE_DIR_VALUE}\""
            "PINGO_SCRIPT_SOURCE_DIR=\"${PINGO_SCRIPT_SOURCE_DIR_VALUE}\""
    )
endif ()

# The scripts of the game, see scripts/Docs.md. They are built as their own
# target that only sees the script API: an engine header cannot be included
# there, so a script can only use what the API offers.
#
# An object library, not a static one: scripts register themselves and are
# never called by name from the engine, so a static library would be dropped
# by the linker.
file(GLOB_RECURSE PINGO_SCRIPT_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/scripts/*.cpp)

if (PINGO_SCRIPT_SOURCES)
    add_library(PingoScripts OBJECT ${PINGO_SCRIPT_SOURCES})

    # Only the API and the other scripts, deliberately not src/
    target_include_directories(PingoScripts PRIVATE
            ${PROJECT_SOURCE_DIR}/src/Scripting/Api
            ${PROJECT_SOURCE_DIR}/scripts
    )

    if (NOT MSVC)
        target_compile_options(PingoScripts PRIVATE -Wall -Wextra)
    endif ()

    target_sources(PingoEngine PRIVATE $<TARGET_OBJECTS:PingoScripts>)
endif ()

# Copy assets into the build directory.
# Not as POST_BUILD, because that only runs when code was recompiled as well:
# a tweaked JSON would otherwise never reach the game. PingoEngine depends
# on this target instead, so the IDE builds it when starting the game.
#
# Deliberately without comparing timestamps: downloaded assets often come
# with an old modification date and would then never be copied.
add_custom_target(CopyAssets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:PingoEngine>/assets
        COMMENT "Kopiere Assets"
)

add_dependencies(PingoEngine CopyAssets)
