// GameData.h - the game data read from the JSON files of the data/ folder
// (version, links, ...). Not in the original: there, these values were
// written in the code.
//
// Usage:
//     takeover::GameData::load("data");          // once, at startup
//     GameData::get().version                    // anywhere afterwards
//
// The files accept // comments.
#pragma once

#include <filesystem>
#include <string>

namespace takeover {

    class GameData {
    public:

        struct Game {
            std::string title;
            std::string version;
        };

        struct Options {
            int width;
            int height;
            bool resizable;
            bool hidden;
            bool vsync;
        };

        /// The links opened by the logos and the intros.
        struct Links {
            std::string armorGames;
            std::string a10;
            std::string iriySoft;
            std::string iriySoftTwitter;
            std::string iriySoftFacebook;
        };

        /// Reads every file of `directory`. Throws std::runtime_error with
        /// the file name when a file is missing or a value is wrong.
        static void load(const std::filesystem::path& directory);

        /// The loaded data. load() must have been called before.
        static const GameData& get();

        // /----- game.json -----/

        Game game;   ///< Shown in the corner of the title screens.
        Options options;
        Links links;
    };

} // namespace takeover
