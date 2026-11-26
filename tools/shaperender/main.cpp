// shaperender - rasterizes SWF shapes to PNG files, for checking the vector
// pipeline (parse -> draw list -> cairo) against reference renders.
//
//   shaperender takeover.swf OUT_DIR [scale] [id...]
//
// Without ids, every DefineShape is rendered. Bitmap fills are resolved from
// the asset pack whose directory is given by the TAKEOVER_ASSETS environment
// variable (output of `assetpack`); without it they render empty. Morph shapes are rendered at
// ratio 0 and 1 (`<id>_0.png`, `<id>_1.png`).

#include "swf/DrawList.h"
#include "swf/Movie.h"
#include "swf/Shape.h"
#include "vector/Rasterizer.h"

#include <cairo.h>
#include <stb/stb_image.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace {

void writePng(const vector::Image& image, const std::filesystem::path& path) {
    cairo_surface_t* s = cairo_image_surface_create_for_data(const_cast<unsigned char*>(image.pixels.data()),
                                                             CAIRO_FORMAT_ARGB32, image.width, image.height,
                                                             image.width * 4);
    cairo_surface_write_to_png(s, path.c_str());
    cairo_surface_destroy(s);
}

/// Loads bitmaps/<id>.png from the asset pack as premultiplied BGRA.
class PackBitmaps {
public:
    const vector::Image* get(std::uint16_t id) {
        const char* root = std::getenv("TAKEOVER_ASSETS");
        if (!root) return nullptr;
        auto [it, inserted] = cache_.try_emplace(id);
        if (inserted) {
            const auto path = std::filesystem::path(root) / "bitmaps" / (std::to_string(id) + ".png");
            int w = 0, h = 0, n = 0;
            if (stbi_uc* rgba = stbi_load(path.c_str(), &w, &h, &n, 4)) {
                it->second = vector::Image(w, h);
                for (int i = 0; i < w * h; ++i) {
                    const unsigned a = rgba[i * 4 + 3];
                    auto pm = [a](unsigned c) { return static_cast<std::uint8_t>((c * a + 127) / 255); };
                    it->second.pixels[i * 4 + 0] = pm(rgba[i * 4 + 2]);
                    it->second.pixels[i * 4 + 1] = pm(rgba[i * 4 + 1]);
                    it->second.pixels[i * 4 + 2] = pm(rgba[i * 4 + 0]);
                    it->second.pixels[i * 4 + 3] = static_cast<std::uint8_t>(a);
                }
                stbi_image_free(rgba);
            }
        }
        return it->second.width > 0 ? &it->second : nullptr;
    }

private:
    std::map<std::uint16_t, vector::Image> cache_;
};

PackBitmaps g_bitmaps;

void render(const swf::DrawList& list, const swf::Rect& bounds, double scale, const std::filesystem::path& path) {
    const int w = std::max(1, static_cast<int>(std::ceil(bounds.width() / 20.0 * scale)));
    const int h = std::max(1, static_cast<int>(std::ceil(bounds.height() / 20.0 * scale)));
    vector::Image image(w, h);
    vector::Affine m;
    m.a = m.d = scale / 20.0;
    m.tx = -bounds.xMin / 20.0 * scale;
    m.ty = -bounds.yMin / 20.0 * scale;
    vector::rasterize(list, m, image, [](std::uint16_t id) { return g_bitmaps.get(id); });
    writePng(image, path);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: shaperender file.swf out_dir [scale] [id...]\n");
        return 2;
    }
    const auto movie = swf::Movie::loadFile(argv[1]);
    const std::filesystem::path outDir = argv[2];
    std::filesystem::create_directories(outDir);
    const double scale = argc > 3 ? std::atof(argv[3]) : 1.0;

    std::vector<swf::CharacterId> ids;
    for (int i = 4; i < argc; ++i) ids.push_back(static_cast<swf::CharacterId>(std::atoi(argv[i])));
    if (ids.empty())
        for (const auto& [id, c] : movie.characters())
            if (std::holds_alternative<swf::ShapeDef>(c)) ids.push_back(id);

    int failures = 0;
    for (const swf::CharacterId id : ids) {
        try {
            if (const auto* shape = movie.get<swf::ShapeDef>(id)) {
                const auto geometry = swf::parseShapeWithStyle(shape->records, shape->version);
                render(swf::buildDrawList(geometry, shape->usesFillWindingRule), shape->bounds, scale,
                       outDir / (std::to_string(id) + ".png"));
            } else if (const auto* morph = movie.get<swf::MorphShapeDef>(id)) {
                const auto geometry = swf::parseMorphShape(morph->records, morph->version);
                render(swf::buildDrawList(swf::interpolateMorph(geometry, 0.0)), morph->startBounds, scale,
                       outDir / (std::to_string(id) + "_0.png"));
                render(swf::buildDrawList(swf::interpolateMorph(geometry, 1.0)), morph->endBounds, scale,
                       outDir / (std::to_string(id) + "_1.png"));
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "shape %u: %s\n", id, e.what());
            ++failures;
        }
    }
    std::printf("rendered %zu shape(s), %d failure(s)\n", ids.size(), failures);
    return failures == 0 ? 0 : 1;
}
