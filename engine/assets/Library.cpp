#include "engine/assets/Library.h"

#include <algorithm>

#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace engine {

namespace {

swf::PathPoint transformPoint(const swf::Matrix& m, double x, double y) {
    return {static_cast<float>(m.a * x + m.c * y + m.tx), static_cast<float>(m.b * x + m.d * y + m.ty)};
}

int lerpInt(int a, int b, double t) { return static_cast<int>(std::lround(a + (b - a) * t)); }

} // namespace

Library::Library(const std::filesystem::path& packDirectory)
    : root_(packDirectory), movie_(swf::Movie::loadFile(packDirectory / "library.swf")) {
    std::ifstream manifestFile(root_ / "manifest.json");
    if (!manifestFile) throw std::runtime_error("missing manifest.json in " + root_.string());
    const nlohmann::json manifest = nlohmann::json::parse(manifestFile);

    stageWidth_ = manifest.at("stage").at("width").get<int>();
    stageHeight_ = manifest.at("stage").at("height").get<int>();
    frameRate_ = manifest.at("stage").at("frameRate").get<double>();

    for (const auto& b : manifest.at("bitmaps"))
        bitmapFiles_[b.at("id").get<swf::CharacterId>()] = root_ / b.at("file").get<std::string>();

    for (const auto& s : manifest.at("sounds")) {
        SoundInfo info;
        info.id = s.at("id").get<swf::CharacterId>();
        info.file = root_ / s.at("file").get<std::string>();
        info.sampleRate = s.at("sampleRate").get<int>();
        info.stereo = s.at("stereo").get<bool>();
        info.sampleCount = s.at("sampleCount").get<std::uint32_t>();
        info.seekSamples = s.value("seekSamples", 0);
        sounds_[info.id] = info;
    }
}

const swf::DrawList* Library::drawList(swf::CharacterId id, std::uint16_t ratio) {
    if (const auto* morph = movie_.get<swf::MorphShapeDef>(id)) {
        const std::uint32_t key = (static_cast<std::uint32_t>(id) << 16) | ratio;
        auto& cached = morphDrawLists_[key];
        if (!cached) {
            auto& geometry = morphs_[id];
            if (!geometry)
                geometry = std::make_unique<swf::MorphShapeGeometry>(swf::parseMorphShape(morph->records, morph->version));
            cached = std::make_unique<swf::DrawList>(
                swf::buildDrawList(swf::interpolateMorph(*geometry, ratio / 65535.0)));
        }
        return cached.get();
    }

    auto& cached = drawLists_[id];
    if (cached) return cached.get();
    if (const auto* shape = movie_.get<swf::ShapeDef>(id)) {
        cached = std::make_unique<swf::DrawList>(
            swf::buildDrawList(swf::parseShapeWithStyle(shape->records, shape->version), shape->usesFillWindingRule));
    } else if (const auto* text = movie_.get<swf::TextDef>(id)) {
        cached = std::make_unique<swf::DrawList>(buildTextDrawList(*text));
    } else {
        drawLists_.erase(id);
        return nullptr;
    }
    return cached.get();
}

swf::Rect Library::bounds(swf::CharacterId id, std::uint16_t ratio) const {
    if (const auto* shape = movie_.get<swf::ShapeDef>(id)) return shape->bounds;
    if (const auto* text = movie_.get<swf::TextDef>(id)) return text->bounds;
    if (const auto* morph = movie_.get<swf::MorphShapeDef>(id)) {
        const double t = ratio / 65535.0;
        const swf::Rect& s = morph->startBounds;
        const swf::Rect& e = morph->endBounds;
        return {lerpInt(s.xMin, e.xMin, t), lerpInt(s.xMax, e.xMax, t), lerpInt(s.yMin, e.yMin, t),
                lerpInt(s.yMax, e.yMax, t)};
    }
    return {};
}

const std::vector<swf::DrawList>* Library::glyphs(swf::CharacterId fontId) {
    const auto* font = movie_.get<swf::FontDef>(fontId);
    if (!font) return nullptr;
    auto& cache = glyphs_[fontId];
    if (cache.empty())
        for (const auto& g : font->glyphShapes) cache.push_back(swf::buildDrawList(swf::parseGlyph(g)));
    return &cache;
}

const swf::FontDef* Library::findFont(std::string_view name, bool bold, bool italic,
                                      std::u32string_view mustContain) const {
    const swf::FontDef* best = nullptr;
    int bestScore = -1;
    for (const auto& [id, character] : movie_.characters()) {
        const auto* font = std::get_if<swf::FontDef>(&character);
        if (!font || font->name != name || font->glyphShapes.empty()) continue;
        int covered = 0;
        for (const char32_t c : mustContain)
            if (std::find(font->codeTable.begin(), font->codeTable.end(), c) != font->codeTable.end()) ++covered;
        // Coverage matters most, then style, then the larger subset.
        const int score = covered * 4 + (font->bold == bold ? 2 : 0) + (font->italic == italic ? 1 : 0);
        if (score > bestScore || (score == bestScore && font->glyphShapes.size() > best->glyphShapes.size())) {
            best = font;
            bestScore = score;
        }
    }
    return best;
}

const swf::FontDef* Library::findFontCovering(std::u32string_view text, bool bold, bool italic) const {
    const swf::FontDef* best = nullptr;
    int bestScore = -1;
    for (const auto& [id, character] : movie_.characters()) {
        const auto* font = std::get_if<swf::FontDef>(&character);
        if (!font || font->glyphShapes.empty()) continue;
        bool coversAll = true;
        for (const char32_t c : text)
            if (std::find(font->codeTable.begin(), font->codeTable.end(), c) == font->codeTable.end()) coversAll = false;
        if (!coversAll) continue;
        const int score = (font->bold == bold ? 2 : 0) + (font->italic == italic ? 1 : 0);
        if (score > bestScore) best = font, bestScore = score;
    }
    return best;
}

swf::DrawList Library::buildTextDrawList(const swf::TextDef& text) {
    // Static text is a list of glyph runs. Each glyph is a font shape in a
    // 1024-unit EM square stored at 20x resolution (DefineFont3), scaled to
    // the run's height and placed on the baseline at the running x position.
    swf::DrawList out;
    const swf::FontDef* font = nullptr;
    const std::vector<swf::DrawList>* glyphs = nullptr;
    swf::Rgba color;
    double height = 0, x = 0, y = 0;

    for (const auto& record : text.records) {
        if (record.fontId) {
            font = movie_.get<swf::FontDef>(*record.fontId);
            glyphs = nullptr;
            if (font) glyphs = this->glyphs(font->id);
        }
        if (record.color) color = *record.color;
        if (record.height) height = *record.height;
        if (record.xOffset) x = *record.xOffset;
        if (record.yOffset) y = *record.yOffset;

        const double scale = height / (1024.0 * 20.0);
        for (const auto& glyph : record.glyphs) {
            if (glyphs && glyph.index < glyphs->size()) {
                for (const auto& command : (*glyphs)[glyph.index].commands) {
                    const auto* fill = std::get_if<swf::FillCommand>(&command);
                    if (!fill) continue;
                    swf::FillCommand placed;
                    placed.style.color = color;
                    for (auto c : fill->path.commands) {
                        c.to = transformPoint(text.matrix, x + c.to.x * scale, y + c.to.y * scale);
                        c.control = transformPoint(text.matrix, x + c.control.x * scale, y + c.control.y * scale);
                        placed.path.commands.push_back(c);
                    }
                    out.commands.emplace_back(std::move(placed));
                }
            }
            x += glyph.advance;
        }
    }
    return out;
}

const vector::Image* Library::bitmap(swf::CharacterId id) {
    auto& cached = bitmaps_[id];
    if (cached) return cached->width > 0 ? cached.get() : nullptr;
    cached = std::make_unique<vector::Image>();
    const auto file = bitmapFiles_.find(id);
    if (file == bitmapFiles_.end()) return nullptr;

    int w = 0, h = 0, channels = 0;
    stbi_uc* rgba = stbi_load(file->second.c_str(), &w, &h, &channels, 4);
    if (!rgba) return nullptr;
    *cached = vector::Image(w, h);
    // PNG holds straight RGBA; the renderer works in premultiplied BGRA.
    for (std::size_t i = 0; i < static_cast<std::size_t>(w) * h; ++i) {
        const unsigned a = rgba[i * 4 + 3];
        auto premultiply = [a](unsigned c) { return static_cast<std::uint8_t>((c * a + 127) / 255); };
        cached->pixels[i * 4 + 0] = premultiply(rgba[i * 4 + 2]);
        cached->pixels[i * 4 + 1] = premultiply(rgba[i * 4 + 1]);
        cached->pixels[i * 4 + 2] = premultiply(rgba[i * 4 + 0]);
        cached->pixels[i * 4 + 3] = static_cast<std::uint8_t>(a);
    }
    stbi_image_free(rgba);
    return cached.get();
}

const SoundInfo* Library::sound(swf::CharacterId id) const {
    const auto it = sounds_.find(id);
    return it == sounds_.end() ? nullptr : &it->second;
}

} // namespace engine
