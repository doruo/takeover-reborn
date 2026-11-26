// Movie.h - a fully parsed SWF file (dictionary + root timeline + linkage).
//
// Usage:
//     swf::Movie movie = swf::Movie::loadFile("takeover.swf");
//     for (const auto& [id, character] : movie.characters()) { ... }
//     const swf::SpriteDef* clip = movie.findSpriteByClass("CastleRedClass");
//
// Movie owns the decompressed file bytes; every swf::Bytes span handed out by
// the definitions points into that buffer. Movie is therefore move-only: a
// move transfers the heap buffer without reallocating, so spans stay valid.
#pragma once

#include "swf/Characters.h"
#include "swf/Timeline.h"
#include "swf/Types.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace swf {

    struct RawTag {
        std::uint16_t code = 0;
        Bytes body;
    };

    struct Header {
        char compression = 'F'; ///< 'F' none, 'C' zlib, 'Z' LZMA.
        int version = 0;
        std::uint32_t fileLength = 0; ///< Uncompressed length, header included.
        Rect frameSize;               ///< Stage size in twips.
        double frameRate = 0;         ///< Frames per second.
        int frameCount = 0;
    };

    class Movie {
    public:
        static Movie loadFile(const std::filesystem::path& path);
        static Movie loadBytes(std::vector<std::uint8_t> fileBytes);

        Movie(Movie&&) noexcept = default;
        Movie& operator=(Movie&&) noexcept = default;
        Movie(const Movie&) = delete;
        Movie& operator=(const Movie&) = delete;

        [[nodiscard]] const Header& header() const { return header_; }
        [[nodiscard]] Rgba backgroundColor() const { return background_; }

        [[nodiscard]] const std::map<CharacterId, Character>& characters() const { return characters_; }
        [[nodiscard]] const Character* find(CharacterId id) const;

        /// Typed lookup: movie.get<ShapeDef>(id) returns nullptr when the id is
        /// unknown or refers to a different kind of character.
        template <typename T>
        [[nodiscard]] const T* get(CharacterId id) const {
            const Character* c = find(id);
            return c ? std::get_if<T>(c) : nullptr;
        }

        [[nodiscard]] const Timeline& rootTimeline() const { return root_; }

        /// AS3 linkage: character id -> fully qualified class name (SymbolClass).
        [[nodiscard]] const std::map<CharacterId, std::string>& symbolClasses() const { return symbolClasses_; }
        [[nodiscard]] std::optional<CharacterId> findClass(const std::string& className) const;

        /// Anti-aliasing hints per text character id.
        [[nodiscard]] const std::unordered_map<CharacterId, TextRenderSettings>& textSettings() const {
            return textSettings_;
        }

        /// Root-level tags in file order (sprite contents stay inside their
        /// DefineSprite body). Used to write a filtered copy of the movie.
        [[nodiscard]] const std::vector<RawTag>& rawTags() const { return rawTags_; }

        /// The encoded stage rect, frame rate and frame count that precede the
        /// first tag in the uncompressed stream.
        [[nodiscard]] Bytes headerBytes() const { return headerBytes_; }

        /// Number of tags of each code, for diagnostics.
        [[nodiscard]] const std::map<std::uint16_t, int>& tagStatistics() const { return tagStats_; }

    private:
        Movie() = default;
        void parse();
        void parseDefinitionTag(const RawTag& tag);

        std::vector<std::uint8_t> data_; ///< Uncompressed bytes after the 8-byte file header.
        Header header_;
        Rgba background_{255, 255, 255, 255};
        std::map<CharacterId, Character> characters_;
        Timeline root_;
        std::map<CharacterId, std::string> symbolClasses_;
        std::unordered_map<std::string, CharacterId> classToId_;
        std::unordered_map<CharacterId, TextRenderSettings> textSettings_;
        Bytes jpegTables_;
        std::map<std::uint16_t, int> tagStats_;
        std::vector<RawTag> rawTags_;
        Bytes headerBytes_;
    };

    /// Serializes a movie made of `headerBytes` (see Movie::headerBytes) and
    /// `tags` into a complete SWF file, zlib-compressed ("CWS"). An End tag is
    /// appended.
    std::vector<std::uint8_t> writeSwf(int version, Bytes headerBytes, const std::vector<RawTag>& tags);

} // namespace swf
