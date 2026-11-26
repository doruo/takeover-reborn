#include "engine/platform/Application.h"

#include "engine/display/MovieClip.h"
#include "engine/platform/Keyboard.h"
#include "engine/platform/Mouse.h"

#include <stb/stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <sstream>

namespace engine {

Application* Application::instance_ = nullptr;

Application::Application(const Options& options) {
    instance_ = this;
    library_ = std::make_unique<Library>(options.assets);

    frameRate_ = library_->frameRate();
    const int scale = std::max(1, options.windowScale);
    window_ = std::make_unique<Window>(Window::Options{.title = options.title,
                                                       .width = library_->stageWidth() * scale,
                                                       .height = library_->stageHeight() * scale,
                                                       .resizable = true,
                                                       .hidden = false,
                                                       .vsync = true});
    audio_ = std::make_unique<AudioSystem>(*library_);

    stage_ = std::make_shared<Stage>(library_->stageWidth(), library_->stageHeight(), frameRate_);
    renderer_ = std::make_unique<Renderer>();
    shapes_ = std::make_unique<ShapeCache>(*library_);
    arrowCursor_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    handCursor_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    if (options.fullscreen) setFullscreen(true);
    if (!options.script.empty()) {
        std::ifstream in(options.script);
        for (std::string line; std::getline(in, line);)
            if (!line.empty() && line[0] != '#') script_.push_back(line);
        fast_ = options.fast;
    }
}

bool Application::screenshot(const std::filesystem::path& file) {
    int width = 0, height = 0;
    window_->framebufferSize(width, height);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::glPixelStorei(GL_PACK_ALIGNMENT, 4);
    gl::glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    // GL rows go bottom-up.
    std::vector<std::uint8_t> flipped(pixels.size());
    const std::size_t row = static_cast<std::size_t>(width) * 4;
    for (int y = 0; y < height; ++y)
        std::copy_n(&pixels[(static_cast<std::size_t>(height) - 1 - y) * row], row, &flipped[static_cast<std::size_t>(y) * row]);
    for (std::size_t i = 3; i < flipped.size(); i += 4) flipped[i] = 255;
    return stbi_write_png(file.string().c_str(), width, height, 4, flipped.data(), width * 4) != 0;
}

vector::Image Application::renderToImage(DisplayObject& source, const Matrix& matrix, const ColorTransform& color,
                                         int width, int height) {
    RenderTarget target(width, height);
    target.bind();
    renderer_->beginOffscreen(width, height);
    RenderContext context{*renderer_, *shapes_, 1};
    source.renderWorld(context, matrix, color);
    vector::Image image = target.readPremultipliedBgra();
    gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return image;
}

void Application::runScriptStep() {
    while (scriptWait_ <= 0 && scriptLine_ < script_.size()) {
        std::istringstream line(script_[scriptLine_++]);
        std::string command;
        line >> command;
        if (command == "wait") {
            line >> scriptWait_;
        } else if (command == "press" || command == "release") {
            double x = 0, y = 0;
            line >> x >> y;
            stage_->mouseMoved(x, y);
            stage_->mouseButton(command == "press");
        } else if (command == "move" || command == "click") {
            double x = 0, y = 0;
            line >> x >> y;
            stage_->mouseMoved(x, y);
            if (command == "click") {
                stage_->mouseButton(true);
                stage_->mouseButton(false);
            }
        } else if (command == "key") {
            unsigned code = 0;
            line >> code;
            stage_->key(true, code, 0, false, false, false);
            stage_->key(false, code, 0, false, false, false);
        } else if (command == "shot") {
            std::string file;
            line >> file;
            renderFrame(false); // read the back buffer before it is swapped
            if (!screenshot(file)) std::fprintf(stderr, "script: cannot write %s\n", file.c_str());
        } else if (command == "pick") {
            // Prints the display object path that would get the mouse at (X, Y).
            double x = 0, y = 0;
            line >> x >> y;
            std::string path;
            for (DisplayObject* o = stage_->targetAt({x, y}); o != nullptr; o = o->parent())
            {
                std::string label = o->name();
                if (auto* clip = dynamic_cast<MovieClip*>(o)) label += "#" + std::to_string(clip->spriteId());
                if (label.empty()) label = "?";
                path = label + (path.empty() ? "" : "." + path);
            }
            std::fprintf(stderr, "pick %g %g: %s\n", x, y, path.c_str());
            if (std::getenv("TAKEOVER_PICK_ALL") != nullptr) {
                // Debug: every object whose shape is under the point.
                std::function<void(DisplayObject&, const std::string&)> walk = [&](DisplayObject& o, const std::string& at) {
                    std::string label = o.name();
                    if (auto* clip = dynamic_cast<MovieClip*>(&o)) label += "#" + std::to_string(clip->spriteId());
                    const std::string here = at + "/" + label;
                    if (o.hitTestPoint(x, y, true)) std::fprintf(stderr, "  hit %s%s\n", here.c_str(), o.visible() ? "" : " (hidden)");
                    if (auto* container = dynamic_cast<DisplayObjectContainer*>(&o))
                        for (int i = 0; i < container->numChildren(); ++i) walk(*container->getChildAt(i), here);
                };
                walk(*stage_, "");
            }
        } else if (command == "resize") {
            // Window size in pixels (to test letterboxing).
            int w = 0, h = 0;
            line >> w >> h;
            SDL_SetWindowSize(window_->handle(), w, h);
        } else if (command == "quit") {
            running_ = false;
            return;
        }
    }
}

Application::~Application() {
    // Display objects may reference the library and the audio system: drop
    // the scene first.
    while (stage_->numChildren() > 0) stage_->removeChildAt(0);
    stage_.reset();
    shapes_.reset();
    renderer_.reset();
    audio_.reset();
    if (arrowCursor_) SDL_DestroyCursor(arrowCursor_);
    if (handCursor_) SDL_DestroyCursor(handCursor_);
    window_.reset();
    library_.reset();
    if (instance_ == this) instance_ = nullptr;
}

void Application::setFullscreen(bool fullscreen) {
    fullscreen_ = fullscreen;
    SDL_SetWindowFullscreen(window_->handle(), fullscreen);
}

Point Application::windowToStage(float x, float y) const {
    // SDL reports window coordinates; the renderer works in framebuffer
    // pixels (they differ on HiDPI screens).
    int windowW = 0, windowH = 0, pixelsW = 0, pixelsH = 0;
    SDL_GetWindowSize(window_->handle(), &windowW, &windowH);
    window_->framebufferSize(pixelsW, pixelsH);
    const double density = windowW > 0 ? static_cast<double>(pixelsW) / windowW : 1;
    const double scale = std::min(static_cast<double>(pixelsW) / stage_->stageWidth(),
                                  static_cast<double>(pixelsH) / stage_->stageHeight());
    const double offsetX = std::round((pixelsW - stage_->stageWidth() * scale) / 2);
    const double offsetY = std::round((pixelsH - stage_->stageHeight() * scale) / 2);
    return {(x * density - offsetX) / scale, (y * density - offsetY) / scale};
}

void Application::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        running_ = false;
        break;
    case SDL_EVENT_MOUSE_MOTION: {
        const Point p = windowToStage(event.motion.x, event.motion.y);
        stage_->mouseMoved(p.x, p.y);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            const Point p = windowToStage(event.button.x, event.button.y);
            stage_->mouseMoved(p.x, p.y);
            stage_->mouseButton(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        stage_->mouseWheel(static_cast<int>(event.wheel.y * 3));
        break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        stage_->mouseMoved(-1, -1);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
        // Window shortcuts, not seen by the game.
        if (pressed && (event.key.key == SDLK_F11 ||
                        (event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0))) {
            setFullscreen(!fullscreen_);
            break;
        }
        const unsigned code = Keyboard::fromSdl(event.key.key);
        if (code == 0) break;
        unsigned charCode = 0;
        if (event.key.key < 128 && event.key.key >= 32) charCode = static_cast<unsigned>(event.key.key);
        stage_->key(pressed, code, charCode, (event.key.mod & SDL_KMOD_SHIFT) != 0,
                    (event.key.mod & SDL_KMOD_CTRL) != 0, (event.key.mod & SDL_KMOD_ALT) != 0);
        break;
    }
    default:
        break;
    }
}

void Application::renderFrame(bool present) {
    int width = 0, height = 0;
    window_->framebufferSize(width, height);
    gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderer_->beginFrame(width, height, stage_->stageWidth(), stage_->stageHeight(), 1, 1, 1);
    // Vector art is rasterized for the current window size: after a big
    // resize, the old textures are the wrong resolution and only waste memory.
    const double scale = renderer_->stageScale();
    if (lastRenderScale_ > 0 && (scale > lastRenderScale_ * 1.5 || scale < lastRenderScale_ / 1.5)) shapes_->clear();
    lastRenderScale_ = scale;

    RenderContext context{*renderer_, *shapes_, scale};
    stage_->render(context, Matrix{}, ColorTransform{});
    if (present) window_->swap();

    const bool hand = Mouse::cursor == Mouse::Cursor::Auto ? stage_->wantsHandCursor()
                                                           : Mouse::cursor != Mouse::Cursor::Arrow;
    if (hand != handShown_) {
        SDL_SetCursor(hand ? handCursor_ : arrowCursor_);
        handShown_ = hand;
    }
}

int Application::run() {
    std::uint64_t previous = SDL_GetTicksNS();
    double accumulatedNs = 0;
    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) handleEvent(event);

        const std::uint64_t now = SDL_GetTicksNS();
        accumulatedNs += static_cast<double>(now - previous);
        previous = now;
        const double frameNs = 1e9 / frameRate_;

        // Run the frames that are due; at most two per display refresh, so a
        // slow machine slows the game down rather than freezing the screen.
        int ran = 0;
        if (!script_.empty()) {
            runScriptStep();
            if (fast_) accumulatedNs = frameNs; // one frame per loop, no waiting
        }
        while (accumulatedNs >= frameNs && ran < 2) {
            if (scriptWait_ > 0) --scriptWait_;
            stage_->advanceFrame();
            audio_->update();
            accumulatedNs -= frameNs;
            ++frames_;
            ++ran;
        }
        if (ran == 2 && accumulatedNs > frameNs) accumulatedNs = 0;

        // Draw only when something may have changed: the game runs at 30 fps
        // even on a 60 Hz screen, and the X220's GPU appreciates the rest.
        if (ran > 0) renderFrame();
        else SDL_Delay(1);
    }
    return 0;
}

} // namespace engine

namespace engine {

void navigateToURL(const std::string& url) { SDL_OpenURL(url.c_str()); }

} // namespace engine
