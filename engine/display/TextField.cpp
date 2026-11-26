#include "engine/display/TextField.h"

#include "engine/assets/Library.h"
#include "engine/render/Renderer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>

namespace engine {

namespace {

// Flash leaves 2 pixels between the field box and the text on every side.
constexpr double kGutter = 2;
// DefineFont3 outlines and metrics use a 1024-unit EM square at 20x.
constexpr double kEmUnits = 1024.0 * 20.0;

// --- UTF-8 ----------------------------------------------------------------------

std::u32string decodeUtf8(std::string_view s) {
    std::u32string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i]);
        int extra = c < 0x80 ? 0 : c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
        char32_t cp = extra == 0 ? c : extra == 1 ? (c & 0x1F) : extra == 2 ? (c & 0x0F) : (c & 0x07);
        ++i;
        for (; extra > 0 && i < s.size(); --extra, ++i) cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
        out.push_back(cp);
    }
    return out;
}

std::string encodeUtf8(std::u32string_view s) {
    std::string out;
    for (const char32_t cp : s) {
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

// --- HTML helpers ---------------------------------------------------------------------

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/// Attributes of a tag such as `font face="Arial" size='12' color=#ff0000`.
std::map<std::string, std::string> parseAttributes(std::string_view tag) {
    std::map<std::string, std::string> attributes;
    std::size_t i = tag.find_first_of(" \t\r\n");
    while (i != std::string_view::npos && i < tag.size()) {
        while (i < tag.size() && std::isspace(static_cast<unsigned char>(tag[i]))) ++i;
        const std::size_t nameStart = i;
        while (i < tag.size() && tag[i] != '=' && !std::isspace(static_cast<unsigned char>(tag[i]))) ++i;
        std::string name = lower(std::string(tag.substr(nameStart, i - nameStart)));
        while (i < tag.size() && std::isspace(static_cast<unsigned char>(tag[i]))) ++i;
        if (i >= tag.size() || tag[i] != '=') {
            if (!name.empty()) attributes[name] = "";
            continue;
        }
        ++i;
        while (i < tag.size() && std::isspace(static_cast<unsigned char>(tag[i]))) ++i;
        std::string value;
        if (i < tag.size() && (tag[i] == '"' || tag[i] == '\'')) {
            const char quote = tag[i++];
            const std::size_t end = tag.find(quote, i);
            value = std::string(tag.substr(i, end - i));
            i = end == std::string_view::npos ? tag.size() : end + 1;
        } else {
            const std::size_t start = i;
            while (i < tag.size() && !std::isspace(static_cast<unsigned char>(tag[i]))) ++i;
            value = std::string(tag.substr(start, i - start));
        }
        attributes[name] = value;
    }
    return attributes;
}

std::uint32_t parseColor(const std::string& value, std::uint32_t fallback) {
    std::string hex = value;
    if (!hex.empty() && hex[0] == '#') hex.erase(0, 1);
    if (hex.empty()) return fallback;
    try {
        return static_cast<std::uint32_t>(std::stoul(hex, nullptr, 16)) & 0xFFFFFF;
    } catch (...) {
        return fallback;
    }
}

TextAlign parseAlign(const std::string& value, TextAlign fallback) {
    const std::string v = lower(value);
    if (v == "left") return TextAlign::Left;
    if (v == "right") return TextAlign::Right;
    if (v == "center") return TextAlign::Center;
    if (v == "justify") return TextAlign::Justify;
    return fallback;
}

char32_t decodeEntity(std::string_view entity) {
    if (entity == "lt") return '<';
    if (entity == "gt") return '>';
    if (entity == "amp") return '&';
    if (entity == "quot") return '"';
    if (entity == "apos") return '\'';
    if (entity == "nbsp") return 0xA0;
    if (entity.size() > 1 && entity[0] == '#') {
        try {
            if (entity[1] == 'x' || entity[1] == 'X') return static_cast<char32_t>(std::stoul(std::string(entity.substr(2)), nullptr, 16));
            return static_cast<char32_t>(std::stoul(std::string(entity.substr(1))));
        } catch (...) {
        }
    }
    return 0;
}

} // namespace

// --- Construction -------------------------------------------------------------------

TextField::TextField(Library& library, const swf::EditTextDef& definition)
    : library_(library), definition_(definition), box_(Rectangle::fromSwf(definition.bounds)) {
    wordWrap_ = definition.wordWrap;
    multiline_ = definition.multiline;
    deviceFont_ = !definition.useOutlines;
    autoSize_ = definition.autoSize ? AutoSize::Left : AutoSize::None;

    if (definition.fontId)
        if (const auto* font = std::get_if<swf::FontDef>(library.find(*definition.fontId))) {
            defaultFormat_.font = font->name;
            defaultFormat_.bold = font->bold;
            defaultFormat_.italic = font->italic;
        }
    if (!definition.fontClass.empty() && defaultFormat_.font.empty()) defaultFormat_.font = definition.fontClass;
    defaultFormat_.size = definition.fontHeight / static_cast<double>(swf::kTwipsPerPixel);
    if (definition.textColor) {
        const swf::Rgba& c = *definition.textColor;
        defaultFormat_.color = (static_cast<std::uint32_t>(c.r) << 16) | (c.g << 8) | c.b;
    }
    if (definition.hasLayout) {
        defaultFormat_.align = static_cast<TextAlign>(definition.align);
        defaultFormat_.leftMargin = definition.leftMargin / 20.0;
        defaultFormat_.rightMargin = definition.rightMargin / 20.0;
        defaultFormat_.indent = definition.indent / 20.0;
        defaultFormat_.leading = definition.leading / 20.0;
    }

    if (definition.html) setHtmlText(definition.initialText);
    else setText(definition.initialText);
}

// --- Content ------------------------------------------------------------------------

std::string TextField::text() const {
    std::u32string all;
    for (const Span& span : spans_) all += span.text;
    return encodeUtf8(all);
}

void TextField::setText(const std::string& text) {
    html_ = text;
    spans_.clear();
    std::u32string decoded = decodeUtf8(text);
    // Flash stores every line break as '\r'.
    std::replace(decoded.begin(), decoded.end(), U'\n', U'\r');
    if (!multiline_ && !wordWrap_) std::erase(decoded, U'\r');
    spans_.push_back({std::move(decoded), defaultFormat_});
    invalidate();
}

void TextField::setHtmlText(const std::string& html) {
    html_ = html;
    parseHtml(html);
    invalidate();
}

void TextField::setTextColor(std::uint32_t rgb) {
    // Like Flash: recolors the whole current text and becomes the default.
    defaultFormat_.color = rgb & 0xFFFFFF;
    for (Span& span : spans_) span.format.color = defaultFormat_.color;
    invalidate();
}

void TextField::parseHtml(const std::string& html) {
    spans_.clear();
    std::vector<TextFormat> stack{defaultFormat_};
    std::u32string pending;
    auto flush = [&] {
        if (pending.empty()) return;
        if (!spans_.empty() && spans_.back().format.font == stack.back().font &&
            spans_.back().format.size == stack.back().size && spans_.back().format.color == stack.back().color &&
            spans_.back().format.bold == stack.back().bold && spans_.back().format.italic == stack.back().italic &&
            spans_.back().format.align == stack.back().align)
            spans_.back().text += pending;
        else
            spans_.push_back({pending, stack.back()});
        pending.clear();
    };
    auto newParagraph = [&] {
        // A paragraph break is '\r'; consecutive <p> don't stack empty lines
        // unless they contain text.
        pending.push_back(U'\r');
    };

    bool hadParagraph = false;
    for (std::size_t i = 0; i < html.size();) {
        const char c = html[i];
        if (c == '<') {
            const std::size_t end = html.find('>', i);
            if (end == std::string::npos) break;
            std::string_view tag(html.data() + i + 1, end - i - 1);
            i = end + 1;
            if (!tag.empty() && tag.back() == '/') tag.remove_suffix(1);
            const bool closing = !tag.empty() && tag[0] == '/';
            if (closing) tag.remove_prefix(1);
            const std::size_t nameEnd = tag.find_first_of(" \t\r\n");
            const std::string name = lower(std::string(tag.substr(0, nameEnd)));
            const auto attributes = parseAttributes(tag);

            if (name == "br") {
                pending.push_back(U'\r');
                continue;
            }
            if (name == "p" || name == "li") {
                if (!closing) {
                    if (hadParagraph) newParagraph();
                    flush();
                    TextFormat format = stack.back();
                    if (auto it = attributes.find("align"); it != attributes.end())
                        format.align = parseAlign(it->second, format.align);
                    stack.push_back(format);
                    hadParagraph = true;
                } else {
                    flush();
                    if (stack.size() > 1) stack.pop_back();
                }
                continue;
            }
            if (name == "font" || name == "b" || name == "i" || name == "u" || name == "textformat" ||
                name == "span" || name == "a") {
                flush();
                if (closing) {
                    if (stack.size() > 1) stack.pop_back();
                    continue;
                }
                TextFormat format = stack.back();
                if (name == "b") format.bold = true;
                if (name == "i") format.italic = true;
                if (name == "u") format.underline = true;
                if (auto it = attributes.find("face"); it != attributes.end()) format.font = it->second;
                if (auto it = attributes.find("size"); it != attributes.end() && !it->second.empty()) {
                    const std::string& v = it->second;
                    const double n = std::atof(v.c_str());
                    format.size = (v[0] == '+' || v[0] == '-') ? format.size + n : n;
                }
                if (auto it = attributes.find("color"); it != attributes.end())
                    format.color = parseColor(it->second, format.color);
                if (auto it = attributes.find("leftmargin"); it != attributes.end())
                    format.leftMargin = std::atof(it->second.c_str());
                if (auto it = attributes.find("rightmargin"); it != attributes.end())
                    format.rightMargin = std::atof(it->second.c_str());
                if (auto it = attributes.find("indent"); it != attributes.end())
                    format.indent = std::atof(it->second.c_str());
                if (auto it = attributes.find("leading"); it != attributes.end())
                    format.leading = std::atof(it->second.c_str());
                stack.push_back(format);
                continue;
            }
            continue; // unknown tags (img, ...) are ignored
        }
        if (c == '&') {
            const std::size_t end = html.find(';', i);
            if (end != std::string::npos && end - i < 10) {
                if (const char32_t decoded = decodeEntity(std::string_view(html).substr(i + 1, end - i - 1))) {
                    pending.push_back(decoded);
                    i = end + 1;
                    continue;
                }
            }
            pending.push_back(U'&');
            ++i;
            continue;
        }
        if (c == '\n' || c == '\r') {
            // A raw line break in htmlText starts a new paragraph in Flash.
            pending.push_back(U'\r');
            i += (c == '\r' && i + 1 < html.size() && html[i + 1] == '\n') ? 2 : 1;
            continue;
        }
        // Copy one UTF-8 sequence.
        std::size_t len = 1;
        const auto byte = static_cast<unsigned char>(c);
        if (byte >= 0xF0) len = 4;
        else if (byte >= 0xE0) len = 3;
        else if (byte >= 0xC0) len = 2;
        pending += decodeUtf8(std::string_view(html).substr(i, len));
        i += len;
    }
    flush();
}

// --- Layout -----------------------------------------------------------------------------

void TextField::invalidate() {
    layoutValid_ = false;
    raster_.reset();
}

const swf::FontDef* TextField::fontFor(const TextFormat& format) const {
    std::u32string sample;
    for (const Span& span : spans_)
        if (span.format.font == format.font) sample += span.text;
    // The field's own embedded font when it matches.
    if (definition_.fontId && !deviceFont_)
        if (const auto* own = std::get_if<swf::FontDef>(library_.find(*definition_.fontId));
            own && own->name == format.font && own->bold == format.bold && !own->glyphShapes.empty())
            return own;
    const auto covers = [&](const swf::FontDef* font) {
        if (font == nullptr) return false;
        for (const char32_t c : sample)
            if (c > U' ' && std::find(font->codeTable.begin(), font->codeTable.end(), c) == font->codeTable.end()) return false;
        return true;
    };
    const swf::FontDef* named = library_.findFont(format.font, format.bold, format.italic, sample);
    if (covers(named)) return named;
    // Device fonts the SWF doesn't embed (or embeds partially): fall back
    // to Arial, then to any embedded font that has the characters.
    const swf::FontDef* arial = library_.findFont("Arial", format.bold, format.italic, sample);
    if (covers(arial)) return arial;
    std::u32string visible;
    for (const char32_t c : sample)
        if (c > U' ') visible += c;
    if (const auto* any = library_.findFontCovering(visible, format.bold, format.italic)) return any;
    return named != nullptr ? named : arial;
}

void TextField::layoutIfNeeded() {
    if (layoutValid_) return;
    layoutValid_ = true;
    glyphs_.clear();
    textWidth_ = textHeight_ = 0;
    lineCount_ = 0;

    // One entry per character, with its font and advance.
    struct Char {
        char32_t code;
        const TextFormat* format;
        const swf::FontDef* font;
        int glyph;       ///< -1 when the font has no such glyph.
        double advance;  ///< Pixels.
    };
    std::map<const TextFormat*, const swf::FontDef*> fonts;
    std::vector<Char> chars;
    for (const Span& span : spans_) {
        const swf::FontDef*& font = fonts[&span.format];
        if (!font) font = fontFor(span.format);
        for (const char32_t code : span.text) {
            Char ch{code, &span.format, font, -1, 0};
            if (font) {
                const auto it = std::find(font->codeTable.begin(), font->codeTable.end(), code == 0xA0 ? U' ' : code);
                if (it != font->codeTable.end()) {
                    ch.glyph = static_cast<int>(it - font->codeTable.begin());
                    if (font->hasLayout && static_cast<std::size_t>(ch.glyph) < font->advances.size())
                        ch.advance = font->advances[static_cast<std::size_t>(ch.glyph)] * span.format.size / kEmUnits;
                }
            }
            if (ch.glyph < 0 && (code == U' ' || code == 0xA0)) ch.advance = span.format.size * 0.25;
            chars.push_back(ch);
        }
    }

    const double usableWidth = box_.width - 2 * kGutter;
    double lineTop = kGutter;
    bool firstLineOfParagraph = true;
    std::size_t index = 0;

    auto metrics = [](const Char& c, double& ascent, double& descent) {
        if (c.font && c.font->hasLayout) {
            ascent = std::max(ascent, c.font->ascent * c.format->size / kEmUnits);
            descent = std::max(descent, c.font->descent * c.format->size / kEmUnits);
        } else {
            ascent = std::max(ascent, c.format->size * 0.9);
            descent = std::max(descent, c.format->size * 0.25);
        }
    };

    // Lay out paragraph by paragraph, line by line.
    do {
        const std::size_t paragraphEnd = [&] {
            std::size_t e = index;
            while (e < chars.size() && chars[e].code != U'\r') ++e;
            return e;
        }();
        const TextFormat& paragraphFormat = index < chars.size() ? *chars[index].format
                                            : !spans_.empty()    ? spans_.back().format
                                                                 : defaultFormat_;
        firstLineOfParagraph = true;
        do {
            const double indent = firstLineOfParagraph ? paragraphFormat.indent : 0;
            const double available =
                usableWidth - paragraphFormat.leftMargin - paragraphFormat.rightMargin - indent;

            // Find where this line ends.
            std::size_t lineEnd = index;
            double width = 0;
            std::size_t lastBreak = std::string::npos;
            while (lineEnd < paragraphEnd) {
                const double advance = chars[lineEnd].advance;
                if (wordWrap_ && width + advance > available && lineEnd > index && chars[lineEnd].code != U' ') {
                    if (lastBreak != std::string::npos) lineEnd = lastBreak + 1; // break after the space
                    break;
                }
                if (chars[lineEnd].code == U' ') lastBreak = lineEnd;
                width += advance;
                ++lineEnd;
            }
            // Measure the line without its trailing spaces.
            double lineWidth = 0, ascent = 0, descent = 0;
            std::size_t visibleEnd = lineEnd;
            while (visibleEnd > index && chars[visibleEnd - 1].code == U' ') --visibleEnd;
            for (std::size_t k = index; k < lineEnd; ++k) {
                if (k < visibleEnd) lineWidth += chars[k].advance;
                metrics(chars[k], ascent, descent);
            }
            if (index == lineEnd) { // empty line: height of its format
                Char probe{U' ', &paragraphFormat, fontFor(paragraphFormat), -1, 0};
                metrics(probe, ascent, descent);
            }

            double x = kGutter + paragraphFormat.leftMargin + indent;
            switch (paragraphFormat.align) {
            case TextAlign::Center: x += std::max(0.0, (available - lineWidth) / 2); break;
            case TextAlign::Right: x += std::max(0.0, available - lineWidth); break;
            default: break;
            }
            const double baseline = lineTop + ascent;
            for (std::size_t k = index; k < lineEnd; ++k) {
                const Char& c = chars[k];
                if (c.glyph >= 0 && c.code != U' ')
                    glyphs_.push_back({c.font, static_cast<std::uint32_t>(c.glyph), box_.x + x, box_.y + baseline,
                                       c.format->size, c.format->color});
                x += c.advance;
            }

            textWidth_ = std::max(textWidth_, lineWidth + paragraphFormat.leftMargin + indent);
            const double leading = paragraphFormat.leading;
            textHeight_ = lineTop - kGutter + ascent + descent;
            lineTop += ascent + descent + leading;
            ++lineCount_;
            index = lineEnd;
            firstLineOfParagraph = false;
        } while (index < paragraphEnd);
        ++index; // skip the '\r'
    } while (index <= chars.size());

    // autoSize grows or shrinks the box around the text.
    if (autoSize_ != AutoSize::None) {
        const double newHeight = textHeight_ + 2 * kGutter;
        if (!wordWrap_) {
            const double newWidth = textWidth_ + 2 * kGutter;
            const double shift = autoSize_ == AutoSize::Center ? (box_.width - newWidth) / 2
                                 : autoSize_ == AutoSize::Right ? box_.width - newWidth
                                                                 : 0;
            box_.x += shift;
            for (PlacedGlyph& g : glyphs_) g.x += shift;
            box_.width = newWidth;
        }
        box_.height = newHeight;
    }
    buildDrawList();
}

void TextField::buildDrawList() {
    drawList_.commands.clear();
    for (const PlacedGlyph& g : glyphs_) {
        const std::vector<swf::DrawList>* outlines = library_.glyphs(g.font->id);
        if (!outlines || g.index >= outlines->size()) continue;
        const double scale = g.size / kEmUnits;
        const float originX = static_cast<float>(g.x * swf::kTwipsPerPixel);
        const float originY = static_cast<float>(g.baseline * swf::kTwipsPerPixel);
        for (const auto& command : (*outlines)[g.index].commands) {
            const auto* fill = std::get_if<swf::FillCommand>(&command);
            if (!fill) continue;
            swf::FillCommand placed;
            placed.nonZeroWinding = fill->nonZeroWinding;
            placed.style.color = {static_cast<std::uint8_t>(g.color >> 16), static_cast<std::uint8_t>(g.color >> 8),
                                  static_cast<std::uint8_t>(g.color), 255};
            // Glyph units -> twips: units * size / EM * 20.
            const auto f = static_cast<float>(scale * swf::kTwipsPerPixel);
            for (auto c : fill->path.commands) {
                c.to = {originX + c.to.x * f, originY + c.to.y * f};
                c.control = {originX + c.control.x * f, originY + c.control.y * f};
                placed.path.commands.push_back(c);
            }
            drawList_.commands.emplace_back(std::move(placed));
        }
    }
}

// --- Geometry ------------------------------------------------------------------------------

double TextField::textWidth() {
    layoutIfNeeded();
    return textWidth_;
}

double TextField::textHeight() {
    layoutIfNeeded();
    return textHeight_;
}

int TextField::numLines() {
    layoutIfNeeded();
    return lineCount_;
}

void TextField::setAutoSize(AutoSize mode) {
    autoSize_ = mode;
    invalidate();
}

void TextField::setWordWrap(bool wrap) {
    wordWrap_ = wrap;
    invalidate();
}

void TextField::setWidth(double width) {
    box_.width = std::max(0.0, width);
    invalidate();
}

void TextField::setHeight(double height) {
    box_.height = std::max(0.0, height);
    invalidate();
}

Rectangle TextField::localBounds() const {
    const_cast<TextField*>(this)->layoutIfNeeded(); // autoSize may change the box
    return box_;
}

bool TextField::hitTestLocal(Point local) const {
    const Rectangle b = localBounds();
    return local.x >= b.x && local.x <= b.right() && local.y >= b.y && local.y <= b.bottom();
}

// --- Drawing ----------------------------------------------------------------------------------

void TextField::draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) {
    layoutIfNeeded();
    if (asMask) return; // text fields are never used as masks in the game
    if (drawList_.commands.empty()) return;

    const swf::Rect bounds{static_cast<std::int32_t>(std::floor(box_.x * 20)),
                           static_cast<std::int32_t>(std::ceil(box_.right() * 20)),
                           static_cast<std::int32_t>(std::floor(box_.y * 20)),
                           static_cast<std::int32_t>(std::ceil((box_.y + std::max(box_.height, textHeight_ + 4)) * 20))};
    const Matrix toDevice = world.then(Matrix::scale(context.deviceScale, context.deviceScale));
    const int bucket = ShapeCache::scaleBucket(bounds, toDevice);
    if (!raster_ || bucket != rasterBucket_) {
        raster_ = std::make_unique<ShapeCache::Entry>(context.shapes.rasterize(drawList_, bounds, toDevice));
        rasterBucket_ = bucket;
    }
    context.renderer.drawTexture(raster_->texture, raster_->textureToLocal.then(world), color);
}

} // namespace engine
