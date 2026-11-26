// SaveFile.h - stores one ByteArray under a name (original:
// _SafePkg_1650/ByteArrayToSharedObject, which used a Flash SharedObject).
//
// The file is <data dir>/<name>.sav, where the data directory is
// $XDG_DATA_HOME/takeover (default ~/.local/share/takeover), or the
// TAKEOVER_SAVE_DIR environment variable when set.
#pragma once

#include "game/framework/storage/ByteArray.h"

#include <filesystem>
#include <optional>
#include <string>

namespace takeover {

class SaveFile {
public:
    explicit SaveFile(std::string name) : name_(std::move(name)) {}

    /// Writes the data, compressed (original: saveData).
    bool saveData(ByteArray data, bool compress);
    /// The stored data, uncompressed (original: _SafeStr_2615).
    [[nodiscard]] std::optional<ByteArray> loadData() const;
    /// True if a complete save exists (original: _SafeStr_2616).
    [[nodiscard]] bool isComplete() const;

    [[nodiscard]] std::filesystem::path path() const;
    static std::filesystem::path directory();

private:
    std::string name_;
};

} // namespace takeover
