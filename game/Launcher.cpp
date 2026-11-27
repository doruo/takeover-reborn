// Launcher.cpp - starts Take Over.
//
//     takeover [--assets DIR] [--data DIR] [--fullscreen] [--scale N] [--cheat]
//
// --cheat turns on the original's developer mode (Preloader.CHEAT_ON):
// two extra spell buttons (mana and gold, all spells) and "quit" in the
// pause menu wins the mission. --script FILE [--fast] runs a test script
// (see engine/platform/Application.h).
//
// The asset pack is looked for in --assets, then $TAKEOVER_ASSETS, then
// next to the executable (../share/takeover/assets for an installed game,
// assets/ in a build tree) and finally ./assets. The data folder (the JSON
// files of game/data/GameData.h) is looked for the same way: --data, then
// $TAKEOVER_DATA, then next to the executable, and finally ./data.
#include "engine/platform/Application.h"
#include "game/Main.h"
#include "game/data/GameData.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Trash.h"
#include "game/level/LevelVisual.h"
#include "game/media/MusicConst.h"
#include "game/media/SoundConst.h"

#include <SDL3/SDL.h>

#if defined(__linux__)
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace {

#if defined(__linux__)
    /// On a crash, prints the call stack (addresses can be turned into lines
    /// with addr2line -e takeover) so bug reports are useful.
    void onCrash(int signal) {
        void* frames[64];
        const int count = backtrace(frames, 64);
        const char header[] = "\nTakeover Reborn crashed. Call stack:\n";

        (void)!write(STDERR_FILENO, header, sizeof header - 1);

        backtrace_symbols_fd(frames, count, STDERR_FILENO);

        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }
#endif

    std::filesystem::path findAssets(const char* fromCommandLine) {
        namespace fs = std::filesystem;

        const auto isPack = 
        [](const fs::path& p) { 
            return fs::exists(p / "manifest.json") 
            && fs::exists(p / "library.swf"); 
        };

        if (fromCommandLine)
            return fromCommandLine;

        if (const char* env = std::getenv("TAKEOVER_ASSETS"); env && *env)
            return env;

        if (const char* base = SDL_GetBasePath()) {
            const fs::path exe(base);
            
            for (const fs::path& candidate : 
                {
                     exe / "../share/takeover/assets",
                     exe / "assets", 
                     exe / "../assets",        
                     exe / "../../assets"
                }
            ) {
                if (isPack(candidate)) {
                    return fs::weakly_canonical(candidate);
                }
            }
        }

        return "assets";
    }

    std::filesystem::path findData(const char* fromCommandLine) {
        namespace fs = std::filesystem;
        const auto isData = [](const fs::path& p) { return fs::exists(p / "game.json"); };

        if (fromCommandLine)
            return fromCommandLine;

        if (const char* env = std::getenv("TAKEOVER_DATA"); env && *env)
            return env;

        if (const char* base = SDL_GetBasePath()) {
            const fs::path exe(base);

            for (const fs::path& candidate : {exe / "../share/takeover/data", exe / "data", exe / "../data",
                                              exe / "../../data"})
                if (isData(candidate))
                    return fs::weakly_canonical(candidate);
        }

        return "data";
    }

} // namespace

int main(int argc, char** argv) {
#if defined(__linux__)
    std::signal(SIGSEGV, onCrash);
    std::signal(SIGABRT, onCrash);
    std::signal(SIGFPE, onCrash);
#endif
    const char* assets = nullptr;
    const char* data = nullptr;
    const char* script = nullptr;

    bool fast = false;
    bool fullscreen = false;
    int scale = 1;


    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--assets") == 0 && i + 1 < argc)
            assets = argv[++i];
        else if (std::strcmp(argv[i], "--data") == 0 && i + 1 < argc)
            data = argv[++i];
        else if (std::strcmp(argv[i], "--fullscreen") == 0)
            fullscreen = true;
        else if (std::strcmp(argv[i], "--scale") == 0 && i + 1 < argc)
            scale = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc)
            script = argv[++i];
        else if (std::strcmp(argv[i], "--fast") == 0)
            fast = true;
        else if (std::strcmp(argv[i], "--cheat") == 0)
            takeover::LevelVisual::mode.add(takeover::LevelVisual::ST_CHEAT_MODE);
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf("usage: takeover [--assets DIR] [--data DIR] [--fullscreen] [--scale N] [--cheat]\n"
                        "  F11 or Alt+Enter toggles fullscreen.\n");
            return 0;
        }
    }

    try {
        const std::filesystem::path assetDir = findAssets(assets);
        if (!std::filesystem::exists(assetDir / "manifest.json")) {
            std::fprintf(stderr,
                         "Asset pack not found in '%s'.\n"
                         "Generate it from the original game with:\n"
                         "  assetpack takeover.swf assets\n",
                         assetDir.string().c_str());
            return 1;
        }

        // Loads data from JSON.
        takeover::GameData::load(findData(data));
        takeover::GameData gameData = takeover::GameData::get();

        engine::Application app(
    {
                .assets = assetDir,
                .title = gameData.game.title,
                .fullscreen = fullscreen,
                .windowScale = scale,
                .script = script ? script : "",
                .fast = fast
            });

        takeover::symbols::registerSymbolScripts();

        auto game = std::make_shared<takeover::Main>();
        app.stage().addChild(game);
        game->init();
        
        const int code = app.run();

        // Tear the game down while the window, GL context and audio device
        // still exist: display objects free GL textures, sounds free voices.
        takeover::MusicConst::instance().manager().dispose();
        takeover::SoundConst::instance().manager().dispose();
        app.stage().removeChild(game.get());
        game.reset();
        takeover::emptyTrash();

        return code;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Take Over stopped: %s\n", e.what());
        return 1;
    }
}
