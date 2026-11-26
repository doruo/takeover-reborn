// Library.h - the asset pack, loaded: symbols, vector art, bitmaps, sounds.
//
// The pack produced by tools/assetpack is a stripped SWF (definitions and
// timelines) plus PNG bitmaps and MP3 sounds. The Library answers the
// questions the display list asks: "what is character 1234?", "give me the
// draw list of this shape at this morph ratio", "pixels of bitmap 42",
// "which character is linkage class CastleRedClass?".
//
// Decoding is lazy: a shape's records are turned into a draw list the first
// time it is displayed, a PNG is read the first time it is used.
#pragma once

#include "swf/DrawList.h"
#include "swf/Movie.h"
#include "vector/Rasterizer.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>

namespace engine {

    struct SoundInfo {
        std::uint16_t id = 0;
        std::filesystem::path file;
        int sampleRate = 44100;
        bool stereo = false;
        std::uint32_t sampleCount = 0;
        int seekSamples = 0;
    };

    class Library {
    public:
        explicit Library(const std::filesystem::path& packDirectory);

        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;

        [[nodiscard]] const swf::Movie& movie() const { return movie_; }
        [[nodiscard]] const swf::Character* find(swf::CharacterId id) const { return movie_.find(id); }
        [[nodiscard]] std::optional<swf::CharacterId> findClass(const std::string& name) const {
            return movie_.findClass(name);
        }

        /// Draw list of a shape (DefineShape), a morph shape at `ratio`
        /// (0..65535), or a static text (DefineText). nullptr for other kinds.
        const swf::DrawList* drawList(swf::CharacterId id, std::uint16_t ratio = 0);

        /// Bounds in twips of anything drawList() accepts.
        [[nodiscard]] swf::Rect bounds(swf::CharacterId id, std::uint16_t ratio = 0) const;

        /// Premultiplied BGRA pixels of a bitmap, or nullptr if unknown.
        const vector::Image* bitmap(swf::CharacterId id);

        [[nodiscard]] const SoundInfo* sound(swf::CharacterId id) const;

        /// Glyph outlines of a font (DefineFont3 EM square, 1024 * 20 units),
        /// indexed like FontDef::codeTable. nullptr if `fontId` isn't a font.
        const std::vector<swf::DrawList>* glyphs(swf::CharacterId fontId);

        /// An embedded font by name, preferring the requested style. Several
        /// fonts may share a name (each text field embeds its own subset), so
        /// `mustContain` picks one that has the characters needed, if any does.
        [[nodiscard]] const swf::FontDef* findFont(std::string_view name, bool bold, bool italic,
                                                   std::u32string_view mustContain = {}) const;
        /// Any embedded font holding every character of `text`, preferring the
        /// requested style; nullptr if none (device text falls back to it).
        [[nodiscard]] const swf::FontDef* findFontCovering(std::u32string_view text, bool bold, bool italic) const;

        /// Stage size in pixels and frame rate of the original movie.
        [[nodiscard]] int stageWidth() const { return stageWidth_; }
        [[nodiscard]] int stageHeight() const { return stageHeight_; }
        [[nodiscard]] double frameRate() const { return frameRate_; }

    private:
        swf::DrawList buildTextDrawList(const swf::TextDef& text);

        std::filesystem::path root_;
        swf::Movie movie_;
        int stageWidth_ = 800;
        int stageHeight_ = 600;
        double frameRate_ = 30;

        std::unordered_map<swf::CharacterId, std::filesystem::path> bitmapFiles_;
        std::unordered_map<swf::CharacterId, std::unique_ptr<vector::Image>> bitmaps_;
        std::unordered_map<swf::CharacterId, SoundInfo> sounds_;

        std::unordered_map<swf::CharacterId, std::unique_ptr<swf::DrawList>> drawLists_;
        std::unordered_map<swf::CharacterId, std::unique_ptr<swf::MorphShapeGeometry>> morphs_;

        /// Morph draw lists keyed by (id << 16 | ratio).
        std::unordered_map<std::uint32_t, std::unique_ptr<swf::DrawList>> morphDrawLists_;
        std::unordered_map<swf::CharacterId, std::vector<swf::DrawList>> glyphs_; ///< Per font.
    };

} // namespace engine
