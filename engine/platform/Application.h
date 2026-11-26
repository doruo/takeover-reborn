// Application.h - the "Flash Player": window, frame loop, input, audio.
//
//     engine::Application app({.assets = "assets", .title = "Take Over"});
//     app.stage().addChild(std::make_shared<Main>(app));
//     return app.run();
//
// The loop is the Flash one: the movie runs at a fixed frame rate (30 fps
// for Take Over). Each frame, timelines advance and ENTER_FRAME is
// broadcast. The game updates itself from there with a fixed dt. If the
// machine can't keep up, the game slows down instead of skipping frames,
// exactly like in the Flash Player.
#pragma once

#include "engine/assets/Library.h"
#include "engine/audio/Sound.h"
#include "engine/display/Stage.h"
#include "engine/platform/Window.h"
#include "engine/render/Renderer.h"
#include "engine/render/ShapeCache.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace engine {

    class Application {
    public:
        struct Options {
            std::filesystem::path assets = "assets";
            std::string title = "Take Over";
            bool fullscreen = false;
            int windowScale = 1; ///< Initial window size = stage size * scale.

            /// Optional test script (see runScriptStep): drives the game with
            /// simulated input and takes screenshots, for automated checks.
            std::filesystem::path script;

            /// Run frames as fast as possible (with a script).
            bool fast = false;
        };

        explicit Application(const Options& options);
        ~Application();
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        /// The running application (one per process).
        static Application& instance() { return *instance_; }

        [[nodiscard]] Library& library() { return *library_; }
        [[nodiscard]] AudioSystem& audio() { return *audio_; }
        [[nodiscard]] Stage& stage() { return *stage_; }

        /// Runs until the window is closed or quit() is called. Returns the exit
        /// code.
        int run();
        void quit() { running_ = false; }

        void setFullscreen(bool fullscreen);
        [[nodiscard]] bool isFullscreen() const { return fullscreen_; }

        /// Changes the frame rate (Flash's stage.frameRate).
        void setFrameRate(double fps) { frameRate_ = fps; }
        [[nodiscard]] double frameRate() const { return frameRate_; }

        /// Number of frames run since the start (handy for debugging).
        [[nodiscard]] std::uint64_t frameCount() const { return frames_; }

        /// Renders `source` with the full transform `matrix` into a transparent
        /// `width` x `height` image (premultiplied BGRA). BitmapData.draw uses it.
        [[nodiscard]] vector::Image renderToImage(DisplayObject& source, const Matrix& matrix, const ColorTransform& color,
                                                  int width, int height);

        /// Writes the last rendered frame to a PNG file.
        bool screenshot(const std::filesystem::path& file);

    private:
        void handleEvent(const SDL_Event& event);

        /// Executes the test script until it waits. Returns false when it is over.
        ///   wait N          let N frames run
        ///   move X Y        mouse to stage coordinates
        ///   click X Y       move, press and release the left button
        ///   press X Y       move and press the left button (release X Y: release)
        ///   key CODE        press and release a key (Flash key code)
        ///   shot FILE.png   screenshot
        ///   pick X Y        print what the mouse would hit there
        ///   resize W H      window size in pixels (letterbox tests)
        ///   quit
        void runScriptStep();
        void renderFrame(bool present = true);

        /// Converts window coordinates to stage coordinates (letterbox aware).
        [[nodiscard]] Point windowToStage(float x, float y) const;

        static Application* instance_;
        std::unique_ptr<Window> window_;
        std::unique_ptr<Library> library_;
        std::unique_ptr<AudioSystem> audio_;
        std::shared_ptr<Stage> stage_;
        std::unique_ptr<Renderer> renderer_;
        std::unique_ptr<ShapeCache> shapes_;
        SDL_Cursor* arrowCursor_ = nullptr;
        SDL_Cursor* handCursor_ = nullptr;
        bool handShown_ = false;
        bool running_ = true;
        bool fullscreen_ = false;
        double frameRate_ = 30;
        std::uint64_t frames_ = 0;
        double lastRenderScale_ = 0;
        std::vector<std::string> script_;
        std::size_t scriptLine_ = 0;
        int scriptWait_ = 0;
        bool fast_ = false;
    };

} // namespace engine

namespace engine {

    /// flash.net.navigateToURL: opens a web page in the user's browser.
    void navigateToURL(const std::string& url);

} // namespace engine
