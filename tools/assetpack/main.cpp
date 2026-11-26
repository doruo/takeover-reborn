// assetpack - converts takeover.swf into the game's asset pack.
//
//   assetpack path/to/takeover.swf path/to/assets
//
// Output:
//   library.swf      every definition and timeline the game displays, without
//                    code (DoABC), bitmaps or sounds. Still a valid SWF, so
//                    swfdump and ffdec can inspect it
//   bitmaps/<id>.png decoded bitmaps, straight alpha (editable)
//   sounds/<id>.mp3  MP3 data exactly as stored in the SWF (editable)
//   manifest.json    stage settings, linkage classes, bitmap and sound tables
//
// Splitting media out of the library keeps the runtime free of image codecs
// other than PNG and lets an artist replace a picture or a sound by
// overwriting one file.

#include "BitmapDecoder.h"

#include "swf/Movie.h"
#include "swf/TagCode.h"

#include <nlohmann/json.hpp>
#include <stb/stb_image_write.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

    /// Tags that never reach the pack's library.swf.
    bool isDroppedTag(std::uint16_t code) {
        switch (static_cast<swf::TagCode>(code)) {
        case swf::TagCode::DoAbc:          // code: ported to C++
        case swf::TagCode::DefineBits:     // bitmaps: bitmaps/*.png
        case swf::TagCode::JpegTables:
        case swf::TagCode::DefineBitsJpeg2:
        case swf::TagCode::DefineBitsJpeg3:
        case swf::TagCode::DefineBitsJpeg4:
        case swf::TagCode::DefineBitsLossless:
        case swf::TagCode::DefineBitsLossless2:
        case swf::TagCode::DefineSound:    // sounds: sounds/*.mp3
        case swf::TagCode::Metadata:       // authoring leftovers
        case swf::TagCode::ProductInfo:
        case swf::TagCode::DebugId:
        case swf::TagCode::EnableDebugger:
        case swf::TagCode::EnableDebugger2:
        case swf::TagCode::EnableTelemetry:
        case swf::TagCode::ScriptLimits:
            return true;
        default:
            return false;
        }
    }

    void writeFile(const fs::path& path, swf::Bytes bytes) {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

        if (!out)
            throw std::runtime_error("cannot write " + path.string());
    }

    const char* soundFormatName(swf::SoundDef::Format format) {
        switch (format) {
        case swf::SoundDef::Format::Mp3: return "mp3";
        case swf::SoundDef::Format::UncompressedNative: return "pcm-native";
        case swf::SoundDef::Format::UncompressedLittleEndian: return "pcm-le";
        case swf::SoundDef::Format::Adpcm: return "adpcm";
        default: return "other";
        }
    }

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: assetpack takeover.swf output_dir\n");
        return 2;
    }

    try {
        const swf::Movie movie = swf::Movie::loadFile(argv[1]);
        const fs::path outDir = argv[2];
        fs::create_directories(outDir / "bitmaps");
        fs::create_directories(outDir / "sounds");

        json manifest;
        manifest["format"] = 1;
        const auto& header = movie.header();
        manifest["stage"] = {{"width", header.frameSize.width() / swf::kTwipsPerPixel},
                             {"height", header.frameSize.height() / swf::kTwipsPerPixel},
                             {"frameRate", header.frameRate},
                             {"background",
                              {movie.backgroundColor().r, movie.backgroundColor().g, movie.backgroundColor().b}}};

        json classes = json::object();

        for (const auto& [id, name] : movie.symbolClasses())
            classes[name] = id;

        manifest["classes"] = classes;

        // /----- BITMAPS -----/

        json bitmaps = json::array();
        int bitmapFailures = 0;

        for (const auto& [id, character] : movie.characters()) {
            const auto* bitmap = std::get_if<swf::BitmapDef>(&character);
            if (!bitmap)
                continue;

            try {
                const assetpack::RgbaImage image = assetpack::decodeBitmap(*bitmap);
                const std::string file = "bitmaps/" + std::to_string(id) + ".png";
                if (!stbi_write_png((outDir / file).c_str(), image.width, image.height, 4, image.pixels.data(),
                                    image.width * 4))
                    throw std::runtime_error("PNG write failed");
                bitmaps.push_back({{"id", id}, {"width", image.width}, {"height", image.height}, {"file", file}});
            } catch (const std::exception& e) {
                std::fprintf(stderr, "bitmap %u: %s\n", id, e.what());
                ++bitmapFailures;
            }
        }

        manifest["bitmaps"] = bitmaps;

        // /----- SOUNDS -----/

        json sounds = json::array();

        for (const auto& [id, character] : movie.characters()) {
            const auto* sound = std::get_if<swf::SoundDef>(&character);
            if (!sound)
                continue;

            json entry = {{"id", id},
                          {"format", soundFormatName(sound->format)},
                          {"sampleRate", sound->sampleRate},
                          {"stereo", sound->stereo},
                          {"sampleCount", sound->sampleCount}};
            if (auto it = movie.symbolClasses().find(id); it != movie.symbolClasses().end())
                entry["className"] = it->second;

            swf::Bytes payload = sound->data;
            if (sound->format == swf::SoundDef::Format::Mp3 && payload.size() >= 2) {
                // MP3 sound data starts with SeekSamples: the encoder delay
                // Flash skips at playback start.
                entry["seekSamples"] = static_cast<std::int16_t>(payload[0] | (payload[1] << 8));
                payload = payload.subspan(2);
            }

            const std::string file = "sounds/" + std::to_string(id) + "." +
                                     (sound->format == swf::SoundDef::Format::Mp3 ? "mp3" : "raw");
            writeFile(outDir / file, payload);
            entry["file"] = file;
            sounds.push_back(std::move(entry));
        }

        manifest["sounds"] = sounds;

        // /----- LIBRARY -----/

        std::vector<swf::RawTag> kept;

        for (const auto& tag : movie.rawTags())
            if (!isDroppedTag(tag.code))
                kept.push_back(tag);
        const auto library = swf::writeSwf(header.version, movie.headerBytes(), kept);
        writeFile(outDir / "library.swf", library);

        std::ofstream(outDir / "manifest.json") << manifest.dump(1) << '\n';

        std::printf("library.swf: %zu tags, %.1f MB\n", kept.size(), static_cast<double>(library.size()) / 1e6);
        std::printf("bitmaps: %zu written, %d failed\n", bitmaps.size(), bitmapFailures);
        std::printf("sounds: %zu written\n", sounds.size());

        return bitmapFailures == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
