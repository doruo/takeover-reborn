#include "game/framework/storage/SaveFile.h"

#include <cstdlib>
#include <fstream>
#include <iterator>

namespace takeover {

namespace {
// File layout: "TOSV", version byte, flags (bit 0 compressed, bit 1 complete), data.
constexpr char kMagic[4] = {'T', 'O', 'S', 'V'};
constexpr std::uint8_t kVersion = 1;
} // namespace

std::filesystem::path SaveFile::directory() {
    if (const char* custom = std::getenv("TAKEOVER_SAVE_DIR"); custom && *custom) return custom;
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) return std::filesystem::path(xdg) / "takeover";
    if (const char* home = std::getenv("HOME"); home && *home)
        return std::filesystem::path(home) / ".local" / "share" / "takeover";
    return "saves";
}

std::filesystem::path SaveFile::path() const { return directory() / (name_ + ".sav"); }

bool SaveFile::saveData(ByteArray data, bool compress) {
    if (compress) data.compress();
    std::error_code error;
    std::filesystem::create_directories(directory(), error);
    // Write a temporary file then rename it, so a crash never leaves a
    // half-written save.
    const std::filesystem::path target = path();
    const std::filesystem::path temporary = target.string() + ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(kMagic, sizeof kMagic);
        const char header[2] = {static_cast<char>(kVersion), static_cast<char>((compress ? 1 : 0) | 2)};
        out.write(header, sizeof header);
        out.write(reinterpret_cast<const char*>(data.bytes().data()), static_cast<std::streamsize>(data.length()));
        if (!out) return false;
    }
    std::filesystem::rename(temporary, target, error);
    return !error;
}

std::optional<ByteArray> SaveFile::loadData() const {
    std::ifstream in(path(), std::ios::binary);
    if (!in) return std::nullopt;
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.size() < 6 || !std::equal(kMagic, kMagic + 4, bytes.begin())) return std::nullopt;
    const bool compressed = (bytes[5] & 1) != 0;
    ByteArray data(std::vector<std::uint8_t>(bytes.begin() + 6, bytes.end()));
    try {
        if (compressed) data.uncompress();
    } catch (const std::exception&) {
        return std::nullopt;
    }
    data.setPosition(0);
    return data;
}

bool SaveFile::isComplete() const {
    std::ifstream in(path(), std::ios::binary);
    char header[6] = {};
    if (!in.read(header, sizeof header)) return false;
    return std::equal(kMagic, kMagic + 4, header) && (header[5] & 2) != 0;
}

} // namespace takeover
