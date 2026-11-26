// clipview - renders a library clip to PNG with the engine, offscreen.
//
//   clipview ASSETS_DIR CLASS_OR_ID [OUT.png] [options]
//
// Without OUT.png, opens a window and plays the clip at the movie's frame
// rate on the 800x600 stage (Escape quits, Space pauses, Right steps).
// With OUT.png, renders one frame offscreen:
//     --frame N      go to frame N (gotoAndStop) before rendering (default 1)
//     --advance N    instead, play N frames from frame 1 (advanceFrame)
//     --scale S      resolution multiplier (default 1)
//     --stage        render on the 800x600 stage with the clip at (0, 0);
//                    default: crop to the clip's bounds (like ffdec exports)
//
// Used to compare our display list against reference renders, and to look
// at any symbol of the game without running it.

#include "engine/assets/Library.h"
#include "engine/display/MovieClip.h"
#include "engine/platform/Window.h"
#include "engine/render/Renderer.h"
#include "engine/render/ShapeCache.h"

#include <stb/stb_image_write.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

    std::uint16_t resolveCharacter(const engine::Library& library, const std::string& what) {
        if (auto classId = library.findClass(what))
            return *classId;

        return static_cast<std::uint16_t>(std::atoi(what.c_str()));
    }

    /// Interactive playback with a fixed timestep: the clip advances exactly
    /// frameRate times per second whatever the display refresh rate.
    int play(const char* assets, const std::string& what) {

        engine::Window window({.title = "clipview - " + what});
        engine::Library library(assets);

        engine::DisplayObjectPtr object = engine::createDisplayObject(library, resolveCharacter(library, what));
        if (!object) {
            std::fprintf(stderr, "'%s' is not a displayable character\n", what.c_str());
            return 1;
        }

        engine::Renderer renderer;
        engine::ShapeCache shapes(library);

        const std::uint64_t frameNs = static_cast<std::uint64_t>(1e9 / library.frameRate());
        std::uint64_t previous = SDL_GetTicksNS(), accumulator = 0;
        bool running = true, paused = false;

        while (running) {

            bool step = false;
            SDL_Event event;

            while (SDL_PollEvent(&event)) {

                if (event.type == SDL_EVENT_QUIT)
                    running = false;

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE)
                        running = false;

                    if (event.key.key == SDLK_SPACE)
                        paused = !paused;

                    if (event.key.key == SDLK_RIGHT)
                        step = true;
                }
            }

            const std::uint64_t now = SDL_GetTicksNS();
            accumulator += now - previous;
            previous = now;

            while (accumulator >= frameNs) {
                if (!paused)
                    object->advanceFrame();

                accumulator -= frameNs;
            }

            if (step)
                object->advanceFrame();

            int width = 0, height = 0;
            window.framebufferSize(width, height);

            gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
            renderer.beginFrame(width, height, library.stageWidth(), library.stageHeight(), 1, 1, 1);
            engine::RenderContext context{renderer, shapes, renderer.stageScale()};
            object->render(context, engine::Matrix{}, engine::ColorTransform{});

            window.swap();
        }

        return 0;
    }

} // namespace

int main(int argc, char** argv) {

    if (argc < 3) {
        std::fprintf(stderr, "usage: clipview ASSETS CLASS_OR_ID [OUT.png [--frame N] [--advance N] [--scale S] [--stage]]\n");
        return 2;
    }

    if (argc == 3) {
        try {
            return play(argv[1], argv[2]);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "error: %s\n", e.what());
            return 1;
        }
    }

    int frame = 1, advance = 0;
    double scale = 1;
    bool stageMode = false;

    for (int i = 4; i < argc; ++i) {

        const std::string option = argv[i];

        if (option == "--frame" && i + 1 < argc)
            frame = std::atoi(argv[++i]);
        else if (option == "--advance" && i + 1 < argc)
            advance = std::atoi(argv[++i]);
        else if (option == "--scale" && i + 1 < argc)
            scale = std::atof(argv[++i]);
        else if (option == "--stage")
            stageMode = true;
    }

    try {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        engine::Window window(
            {
                .title = "clipview",
                .width = 64,
                .height = 64,
                .resizable = false,
                .hidden = true,
                .vsync = false
            });

        engine::Library library(argv[1]);

        const std::string what = argv[2];
        const std::uint16_t id = resolveCharacter(library, what);

        engine::DisplayObjectPtr object = engine::createDisplayObject(library, id);

        if (!object) {
            std::fprintf(stderr, "'%s' is not a displayable character\n", what.c_str());
            return 1;
        }

        auto* clip = dynamic_cast<engine::MovieClip*>(object.get());

        if (clip) {
            if (advance > 0)
                for (int i = 0; i < advance; ++i)
                    clip->advanceFrame();
            else
                clip->gotoAndStop(frame);
        }

        // Output geometry: stage or the clip's bounds.
        engine::Rectangle area{
            0, 0, static_cast<double>(library.stageWidth()), static_cast<double>(library.stageHeight())
        };

        if (!stageMode)
            area = object->localBounds();

        const int width = std::max(1, static_cast<int>(std::ceil(area.width * scale)));
        const int height = std::max(1, static_cast<int>(std::ceil(area.height * scale)));

        engine::RenderTarget target(width, height);
        engine::Renderer renderer;
        engine::ShapeCache shapes(library);

        target.bind();

        // Transparent background outside stage mode, to compare with exports.
        renderer.beginFrame(width, height, width, height, 0, 0, 0);

        gl::glClearColor(0, 0, 0, 0);
        gl::glClear(GL_COLOR_BUFFER_BIT);

        if (stageMode) {
            gl::glClearColor(1, 1, 1, 1);
            gl::glClear(GL_COLOR_BUFFER_BIT);
        }

        engine::RenderContext context{renderer, shapes, 1.0};
        const engine::Matrix view = engine::Matrix::translation(-area.x, -area.y).then(engine::Matrix::scale(scale, scale));
        object->render(context, view, engine::ColorTransform{});

        const auto pixels = target.readRgba();
        if (!stbi_write_png(argv[3], width, height, 4, pixels.data(), width * 4)) {
            std::fprintf(stderr, "cannot write %s\n", argv[3]);
            return 1;
        }

        std::printf("%s: %dx%d, %zu shape texture(s)%s\n", argv[3], width, height, shapes.size(),
                    clip ? (", frame " + std::to_string(clip->currentFrame()) + "/" +
                    std::to_string(clip->totalFrames())
                ).c_str(): "");

    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    return 0;
}
