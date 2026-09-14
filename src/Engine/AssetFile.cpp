#include "AssetFile.h"

#include <filesystem>
#include <fstream>
#include <system_error>

bool WriteFileAtomically(const std::string &path, const std::string &text) {
    std::filesystem::path target(path);
    std::error_code ignored;

    if (target.has_parent_path()) {
        std::filesystem::create_directories(target.parent_path(), ignored);
    }

    std::string temporary = path + ".tmp";

    {
        std::ofstream file(temporary, std::ios::binary);

        file << text;
        file.flush();

        if (!file) {
            std::filesystem::remove(temporary, ignored);
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, target, error);

    return !error;
}

bool SaveAsset(const std::string &relativePath, const std::string &text) {
    bool saved = WriteFileAtomically("assets/" + relativePath, text);

#ifdef PINGO_ASSET_SOURCE_DIR
    saved = WriteFileAtomically(std::string(PINGO_ASSET_SOURCE_DIR) + "/" + relativePath, text) && saved;
#endif

    return saved;
}

bool AssetExists(const std::string &relativePath) {
    std::error_code ignored;

    if (std::filesystem::exists("assets/" + relativePath, ignored)) {
        return true;
    }

#ifdef PINGO_ASSET_SOURCE_DIR
    if (std::filesystem::exists(std::string(PINGO_ASSET_SOURCE_DIR) + "/" + relativePath, ignored)) {
        return true;
    }
#endif

    return false;
}

bool DeleteAsset(const std::string &relativePath) {
    std::error_code ignored;

    std::filesystem::remove("assets/" + relativePath, ignored);

#ifdef PINGO_ASSET_SOURCE_DIR
    std::filesystem::remove(std::string(PINGO_ASSET_SOURCE_DIR) + "/" + relativePath, ignored);
#endif

    return !AssetExists(relativePath);
}
