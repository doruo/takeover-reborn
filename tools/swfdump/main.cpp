// swfdump - prints what a SWF file contains. A debugging companion to the
// asset converter: use it to inspect a symbol before porting the code that
// drives it.
//
//   swfdump takeover.swf                 summary + tag statistics
//   swfdump takeover.swf --classes       every linkage class and its id
//   swfdump takeover.swf --sprite 1234   frames of one sprite
//   swfdump takeover.swf --class Name    same, by linkage class name
//   swfdump takeover.swf --texts         every dynamic text field and its settings

#include "swf/Movie.h"
#include "swf/TagCode.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace {

const char* kindName(const swf::Character& c) {
    return std::visit(
        [](const auto& def) -> const char* {
            using T = std::decay_t<decltype(def)>;
            if constexpr (std::is_same_v<T, swf::ShapeDef>) return "shape";
            else if constexpr (std::is_same_v<T, swf::MorphShapeDef>) return "morph";
            else if constexpr (std::is_same_v<T, swf::BitmapDef>) return "bitmap";
            else if constexpr (std::is_same_v<T, swf::SpriteDef>) return "sprite";
            else if constexpr (std::is_same_v<T, swf::ButtonDef>) return "button";
            else if constexpr (std::is_same_v<T, swf::EditTextDef>) return "edittext";
            else if constexpr (std::is_same_v<T, swf::TextDef>) return "text";
            else if constexpr (std::is_same_v<T, swf::FontDef>) return "font";
            else return "sound";
        },
        c);
}

void printTexts(const swf::Movie& movie) {
    static const char* kAlign[] = {"left", "right", "center", "justify"};
    for (const auto& [id, character] : movie.characters()) {
        const auto* t = std::get_if<swf::EditTextDef>(&character);
        if (!t) continue;
        std::string font = "-";
        if (t->fontId)
            if (const auto* f = std::get_if<swf::FontDef>(movie.find(*t->fontId))) font = f->name;
        std::printf("%5u %-24s %5.1fpx %-7s %s%s%s%s%s%s var=%s text=\"%.60s\"\n", id, font.c_str(),
                    t->fontHeight / 20.0, kAlign[static_cast<int>(t->align)], t->html ? "html " : "",
                    t->multiline ? "multi " : "", t->wordWrap ? "wrap " : "", t->autoSize ? "auto " : "",
                    t->useOutlines ? "embed " : "device ", t->readOnly ? "ro" : "input", t->variableName.c_str(),
                    t->initialText.c_str());
    }
}

void printDisplayStats(const swf::Movie& movie) {
    static const char* kFilters[] = {"DropShadow", "Blur", "Glow", "Bevel", "Gradient", "Convolution", "ColorMatrix"};
    std::map<int, int> blends;
    std::map<std::string, int> filters;
    for (const auto& [id, character] : movie.characters()) {
        const auto* sprite = std::get_if<swf::SpriteDef>(&character);
        if (!sprite) continue;
        for (const auto& frame : sprite->timeline.frames)
            for (const auto& command : frame.commands) {
                const auto* place = std::get_if<swf::PlaceObject>(&command);
                if (!place) continue;
                if (place->blendMode) ++blends[static_cast<int>(*place->blendMode)];
                if (place->filters)
                    for (const auto& f : *place->filters) {
                        std::string name = kFilters[f.index()];
                        if (const auto* g = std::get_if<swf::GlowFilter>(&f)) {
                            if (g->inner) name += " inner";
                            if (g->knockout) name += " knockout";
                        }
                        if (const auto* d = std::get_if<swf::DropShadowFilter>(&f)) {
                            if (d->inner) name += " inner";
                            if (d->knockout) name += " knockout";
                        }
                        ++filters[name];
                    }
            }
    }
    for (const auto& [mode, count] : blends) std::printf("blend %2d: %d\n", mode, count);
    for (const auto& [name, count] : filters) std::printf("filter %-24s %d\n", name.c_str(), count);
}

void printSummary(const swf::Movie& movie) {
    const auto& h = movie.header();
    std::printf("SWF v%d (%c), stage %dx%d px, %.2f fps, %d root frame(s)\n", h.version, h.compression,
                h.frameSize.width() / swf::kTwipsPerPixel, h.frameSize.height() / swf::kTwipsPerPixel, h.frameRate,
                h.frameCount);

    std::map<std::string, int> kinds;
    for (const auto& [id, c] : movie.characters()) ++kinds[kindName(c)];
    std::printf("\nCharacters:\n");
    for (const auto& [kind, count] : kinds) std::printf("  %-9s %6d\n", kind.c_str(), count);
    std::printf("  linkage classes: %zu\n", movie.symbolClasses().size());

    std::printf("\nTags:\n");
    for (const auto& [code, count] : movie.tagStatistics())
        std::printf("  %-30.*s %7d\n", static_cast<int>(swf::tagName(code).size()), swf::tagName(code).data(), count);
}

void printSprite(const swf::Movie& movie, swf::CharacterId id) {
    const auto* sprite = movie.get<swf::SpriteDef>(id);
    if (!sprite) {
        std::fprintf(stderr, "character %u is not a sprite\n", id);
        std::exit(1);
    }
    std::printf("sprite %u: %d frame(s)\n", id, sprite->timeline.frameCount());
    int frameNumber = 1;
    for (const auto& frame : sprite->timeline.frames) {
        std::printf("frame %d", frameNumber++);
        for (const auto& label : frame.labels) std::printf("  [label \"%s\"]", label.c_str());
        std::printf("\n");
        for (const auto& command : frame.commands) {
            if (const auto* p = std::get_if<swf::PlaceObject>(&command)) {
                std::printf("  %s depth=%u", p->isMove ? "move " : "place", p->depth);
                if (p->characterId) {
                    const auto* c = movie.find(*p->characterId);
                    std::printf(" char=%u(%s)", *p->characterId, c ? kindName(*c) : "?");
                }
                if (p->name) std::printf(" name=\"%s\"", p->name->c_str());
                if (p->matrix)
                    std::printf(" m=[%.3f %.3f %.3f %.3f %.1f %.1f]", p->matrix->a, p->matrix->b, p->matrix->c,
                                p->matrix->d, p->matrix->tx / 20.0, p->matrix->ty / 20.0);
                if (p->ratio) std::printf(" ratio=%u", *p->ratio);
                if (p->clipDepth) std::printf(" clipDepth=%u", *p->clipDepth);
                if (p->filters) std::printf(" filters=%zu", p->filters->size());
                if (p->blendMode) std::printf(" blend=%d", static_cast<int>(*p->blendMode));
                if (p->colorTransform && !p->colorTransform->isIdentity()) std::printf(" cxform");
                std::printf("\n");
            } else {
                std::printf("  remove depth=%u\n", std::get<swf::RemoveObject>(command).depth);
            }
        }
        for (const auto& s : frame.sounds) std::printf("  sound %u\n", s.soundId);
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: swfdump file.swf [--classes | --sprite ID | --class NAME]\n");
        return 2;
    }
    try {
        const swf::Movie movie = swf::Movie::loadFile(argv[1]);
        const std::string_view option = argc > 2 ? argv[2] : "";
        if (option.empty()) {
            printSummary(movie);
        } else if (option == "--display-stats") {
            printDisplayStats(movie);
        } else if (option == "--texts") {
            printTexts(movie);
        } else if (option == "--classes") {
            for (const auto& [id, name] : movie.symbolClasses()) {
                const auto* c = movie.find(id);
                std::printf("%6u %-9s %s\n", id, c ? kindName(*c) : "-", name.c_str());
            }
        } else if (option == "--sprite" && argc > 3) {
            printSprite(movie, static_cast<swf::CharacterId>(std::atoi(argv[3])));
        } else if (option == "--class" && argc > 3) {
            const auto id = movie.findClass(argv[3]);
            if (!id) {
                std::fprintf(stderr, "no class named %s\n", argv[3]);
                return 1;
            }
            printSprite(movie, *id);
        } else {
            std::fprintf(stderr, "unknown option\n");
            return 2;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
