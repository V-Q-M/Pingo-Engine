#pragma once

// See Config for the differences. The mode is chosen in CMake via
// PINGO_BUILD_MODE, in CLion via the CMake profile.
enum class BuildMode {
    Release,
    Debugging,
    Development
};
