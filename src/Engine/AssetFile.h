#pragma once

#include <string>
#include <vector>

// Writes text to a file: first to a temporary file, then renames it.
// If writing fails, the old file stays intact. Missing folders are
// created.
bool WriteFileAtomically(const std::string &path, const std::string &text);

// Saves an asset, path relative to the asset folder. It is written to the
// copy in the build directory, which the game reads from, and in the
// Development build also to the asset folder of the sources. Only then does
// the change survive the next build.
bool SaveAsset(const std::string &relativePath, const std::string &text);

// Does the asset exist, in the build directory or, in the Development build,
// also in the asset folder of the sources?
bool AssetExists(const std::string &relativePath);

// Deletes an asset in both places, see SaveAsset. true if it no longer
// exists anywhere afterwards.
bool DeleteAsset(const std::string &relativePath);

// The files with this extension in a folder of the assets, as paths relative to
// the asset folder, alphabetical, e.g. "sprites/tilesets/tiles_atlas.png" for
// AssetFilesIn("sprites/tilesets", ".png")
std::vector<std::string> AssetFilesIn(const std::string &folder, const std::string &extension);
