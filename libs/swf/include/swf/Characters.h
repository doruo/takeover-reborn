// Characters.h - the "dictionary" entries of a SWF file.
//
// A SWF movie is a list of definitions (shapes, bitmaps, sprites, fonts, ...)
// identified by a 16-bit CharacterId, plus timelines that place those
// characters on the display list frame after frame.
//
// Heavy binary payloads (shape records, image data, sound samples) are kept as
// std::span views into the decompressed file buffer owned by swf::Movie. They
// are only valid while that Movie is alive.
#pragma once

#include "swf/Timeline.h"
#include "swf/Types.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace swf {

    using Bytes = std::span<const std::uint8_t>;

    struct ShapeDef {
        CharacterId id = 0;
        int version = 1; ///< 1..4 for DefineShape..DefineShape4.
        Rect bounds;     ///< Includes half the stroke widths.
        Rect edgeBounds; ///< Geometry only (DefineShape4); equals bounds otherwise.
        bool usesFillWindingRule = false;
        Bytes records; ///< SHAPEWITHSTYLE, starting with the fill style array.
    };

    struct MorphShapeDef {
        CharacterId id = 0;
        int version = 1;
        Rect startBounds;
        Rect endBounds;
        Bytes records; ///< Everything after the bounds (offset, styles, edges).
    };

    struct BitmapDef {
        enum class Kind {
            Jpeg,          ///< DefineBits: JPEG data that needs the shared JPEGTables.
            EncodedImage,  ///< DefineBitsJPEG2: complete JPEG, PNG or GIF file.
            JpegWithAlpha, ///< DefineBitsJPEG3: JPEG plus a zlib-compressed alpha plane.
            Lossless,      ///< DefineBitsLossless: zlib RGB data.
            LosslessAlpha, ///< DefineBitsLossless2: zlib premultiplied ARGB data.
        };

        CharacterId id = 0;
        Kind kind = Kind::EncodedImage;
        Bytes imageData; ///< Encoded image (JPEG kinds) or zlib pixel data (lossless kinds).
        Bytes alphaData; ///< zlib alpha plane, JpegWithAlpha only.
        Bytes jpegTables; ///< Kind::Jpeg only: the JPEGTables in effect when defined.

        // Lossless kinds only.
        int losslessFormat = 0; ///< 3 = 8-bit palette, 4 = 15-bit RGB, 5 = 32-bit.
        int width = 0;
        int height = 0;
        int colorTableSize = 0; ///< Number of palette entries minus one (format 3).
    };

    struct SpriteDef {
        CharacterId id = 0;
        Timeline timeline;
    };

    struct ButtonRecord {
        bool stateUp = false;
        bool stateOver = false;
        bool stateDown = false;
        bool stateHitTest = false;
        CharacterId characterId = 0;
        Depth depth = 0;
        Matrix matrix;
        ColorTransform colorTransform;
        std::vector<Filter> filters;
        BlendMode blendMode = BlendMode::Normal;
    };

    struct ButtonDef {
        CharacterId id = 0;
        bool trackAsMenu = false;
        std::vector<ButtonRecord> records;
    };

    struct EditTextDef {
        enum class Align : std::uint8_t { Left = 0, Right = 1, Center = 2, Justify = 3 };

        CharacterId id = 0;
        Rect bounds;
        bool wordWrap = false;
        bool multiline = false;
        bool password = false;
        bool readOnly = false;
        bool autoSize = false;
        bool noSelect = false;
        bool border = false;
        bool wasStatic = false;
        bool html = false;
        bool useOutlines = false; ///< true = embedded font glyphs, false = device font.

        std::optional<CharacterId> fontId;
        std::string fontClass;
        int fontHeight = 0; ///< Twips.
        std::optional<Rgba> textColor;
        int maxLength = 0;

        bool hasLayout = false;
        Align align = Align::Left;
        int leftMargin = 0;  ///< Twips.
        int rightMargin = 0; ///< Twips.
        int indent = 0;      ///< Twips.
        int leading = 0;     ///< Twips, may be negative.

        std::string variableName;
        std::string initialText; ///< HTML when `html` is set.
    };

    struct GlyphEntry {
        std::uint32_t index = 0; ///< Index into the font's glyph table.
        std::int32_t advance = 0; ///< Twips.
    };

    struct TextRecord {
        std::optional<CharacterId> fontId;
        std::optional<Rgba> color;
        std::optional<std::int16_t> xOffset;
        std::optional<std::int16_t> yOffset;
        std::optional<std::uint16_t> height; ///< Twips; present whenever fontId is.
        std::vector<GlyphEntry> glyphs;
    };

    /// Static text (DefineText / DefineText2).
    struct TextDef {
        CharacterId id = 0;
        Rect bounds;
        Matrix matrix;
        std::vector<TextRecord> records;
    };

    struct KerningRecord {
        std::uint16_t code1 = 0;
        std::uint16_t code2 = 0;
        std::int16_t adjustment = 0;
    };

    /// DefineFont3: glyph outlines are in EM-square units of 1024 * 20.
    struct FontDef {
        CharacterId id = 0;
        std::string name;
        std::string displayName; ///< From DefineFontName, if present.
        bool bold = false;
        bool italic = false;
        bool smallText = false;
        int languageCode = 0;

        std::vector<Bytes> glyphShapes; ///< SHAPE records, one per glyph.
        std::vector<std::uint16_t> codeTable; ///< UCS-2 code point of each glyph.

        bool hasLayout = false;
        int ascent = 0;
        int descent = 0;
        int leading = 0;
        std::vector<std::int16_t> advances;
        std::vector<Rect> glyphBounds;
        std::vector<KerningRecord> kerning;
    };

    struct SoundDef {
        enum class Format : std::uint8_t {
            UncompressedNative = 0,
            Adpcm = 1,
            Mp3 = 2,
            UncompressedLittleEndian = 3,
            Nellymoser16k = 4,
            Nellymoser8k = 5,
            Nellymoser = 6,
            Speex = 11,
        };

        CharacterId id = 0;
        Format format = Format::Mp3;
        int sampleRate = 44100; ///< Hz, decoded from the 2-bit rate field.
        bool is16Bit = true;
        bool stereo = false;
        std::uint32_t sampleCount = 0; ///< Per channel.
        Bytes data; ///< For MP3: SeekSamples (SI16) followed by MP3 frames.
    };

    /// Typographic hints attached to a text field (CSMTextSettings).
    struct TextRenderSettings {
        bool useAdvancedAntiAliasing = false;
        int gridFit = 0; ///< 0 none, 1 pixel, 2 subpixel.
        float thickness = 0;
        float sharpness = 0;
    };

    using Character =
        std::variant<ShapeDef, MorphShapeDef, BitmapDef, SpriteDef, ButtonDef, EditTextDef, TextDef, FontDef, SoundDef>;

} // namespace swf
