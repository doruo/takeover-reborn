// TextField.h - flash.text.TextField (dynamic text) with embedded fonts.
//
// What it reproduces from Flash:
//   * the field box and its 2-pixel gutter on every side,
//   * the htmlText subset the game uses: <p align>, <font face size color>,
//     <b>, <i>, <u>, <br>, <textformat>, <span>, entities,
//   * paragraphs, word wrap, left/center/right alignment, margins, indent,
//     leading, autoSize,
//   * glyphs drawn from the fonts embedded in the SWF (their outlines are
//     regular SWF shapes, rasterized like any other vector art). Fields that
//     asked for a device font use the embedded font of the same name.
//
// Text is not editable and not selectable: the game has no input fields.
#pragma once

#include "engine/display/DisplayObject.h"
#include "engine/render/ShapeCache.h"
#include "swf/Characters.h"
#include "swf/DrawList.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine {

    class Library;

    enum class TextAlign { Left, Right, Center, Justify };

    /// flash.text.TextFormat (only the properties in use).
    struct TextFormat {
        std::string font;
        double size = 12;              ///< Pixels.
        std::uint32_t color = 0x000000; ///< 0xRRGGBB.
        bool bold = false;
        bool italic = false;
        bool underline = false;
        TextAlign align = TextAlign::Left;
        double leftMargin = 0;
        double rightMargin = 0;
        double indent = 0;
        double leading = 0;
    };

    class TextField : public DisplayObject {
    public:
        enum class AutoSize { None, Left, Center, Right };

        /// A field placed by a timeline (DefineEditText).
        TextField(Library& library, const swf::EditTextDef& definition);

        // /----- CONTENT -----/

        /// Plain text. Paragraphs are separated by '\r' like in Flash.
        [[nodiscard]] std::string text() const;
        void setText(const std::string& text);
        void appendText(const std::string& text) { setText(this->text() + text); }

        /// The HTML last assigned (Flash regenerates its own markup, the game
        /// only reads back what it wrote).
        [[nodiscard]] const std::string& htmlText() const { return html_; }
        void setHtmlText(const std::string& html);

        [[nodiscard]] std::uint32_t textColor() const { return defaultFormat_.color; }
        void setTextColor(std::uint32_t rgb);

        [[nodiscard]] const TextFormat& defaultTextFormat() const { return defaultFormat_; }
        void setDefaultTextFormat(const TextFormat& format) { defaultFormat_ = format; }

        // /----- LAYOUT -----/

        [[nodiscard]] AutoSize autoSize() const { return autoSize_; }
        void setAutoSize(AutoSize mode);
        [[nodiscard]] bool wordWrap() const { return wordWrap_; }
        void setWordWrap(bool wrap);
        [[nodiscard]] bool multiline() const { return multiline_; }
        void setMultiline(bool value) { multiline_ = value; }
        void setSelectable(bool) {} // no selection support: nothing to do

        /// Size of the laid out text, without the gutter.
        [[nodiscard]] double textWidth();
        [[nodiscard]] double textHeight();
        [[nodiscard]] int numLines();

        /// Setting width/height resizes the box (it doesn't scale the text).
        void setWidth(double width) override;
        void setHeight(double height) override;

        [[nodiscard]] Rectangle localBounds() const override;
        [[nodiscard]] bool hitTestLocal(Point local) const override;
        [[nodiscard]] bool isInteractive() const override { return true; }

    protected:
        void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

    private:
        /// A run of characters sharing one format.
        struct Span {
            std::u32string text;
            TextFormat format;
        };

        struct PlacedGlyph {
            const swf::FontDef* font = nullptr;
            std::uint32_t index = 0;
            double x = 0, baseline = 0, size = 0;
            std::uint32_t color = 0;
        };

        void parseHtml(const std::string& html);
        void invalidate();
        void layoutIfNeeded();
        [[nodiscard]] const swf::FontDef* fontFor(const TextFormat& format) const;
        void buildDrawList();

        Library& library_;
        const swf::EditTextDef& definition_;
        Rectangle box_;
        TextFormat defaultFormat_;
        std::vector<Span> spans_;
        std::string html_;
        AutoSize autoSize_ = AutoSize::None;
        bool wordWrap_ = false;
        bool multiline_ = false;
        bool deviceFont_ = false;

        // Layout results.
        bool layoutValid_ = false;
        std::vector<PlacedGlyph> glyphs_;
        double textWidth_ = 0, textHeight_ = 0;
        int lineCount_ = 0;
        swf::DrawList drawList_;
        std::unique_ptr<ShapeCache::Entry> raster_;
        int rasterBucket_ = 0;
    };

} // namespace engine
