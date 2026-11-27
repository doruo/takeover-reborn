#include "game/data/GameData.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace takeover {

    using Json = nlohmann::json;
    using Path = std::filesystem::path;

    namespace {

        GameData& instance() {
            static GameData data;
            return data;
        }

        bool loaded = false;

        /// Parses a JSON file.
        Json readJson(const Path& file) {
            std::ifstream stream(file);

            if (!stream)
                throw std::runtime_error("missing data file " + file.string());

            try {
                return Json::parse(
                    stream, 
                    nullptr, 
                    true,
                    true
                );
            } 
            catch (const Json::exception& error) {
                throw std::runtime_error(file.string() + ": " + error.what());
            }
        }

        // /----- game.json -----/

        void readGame(const Path& file, GameData& data) {
            const Json json = readJson(file);

            try {
                // Game
                const Json game = json.at("game");

                data.game.title = game.at("title").get<std::string>();
                data.game.version = game.at("version").get<std::string>();

                // Options
                const Json& options = json.at("options");

                data.options.width = options.at("width").get<int>();
                data.options.height = options.at("height").get<int>();
                data.options.resizable = options.at("resizable").get<bool>();
                data.options.hidden = options.at("hidden").get<bool>();
                data.options.vsync = options.at("vsync").get<bool>();

                // Links
                const Json& links = json.at("links");

                data.links.armorGames = links.at("armorGames").get<std::string>();
                data.links.a10 = links.at("a10").get<std::string>();
                data.links.iriySoft = links.at("iriySoft").get<std::string>();
                data.links.iriySoftTwitter = links.at("iriySoftTwitter").get<std::string>();
                data.links.iriySoftFacebook = links.at("iriySoftFacebook").get<std::string>();
            } 
            catch (const Json::exception& error) {
                throw std::runtime_error(file.string() + ": " + error.what());
            }
        }

    } // namespace

    void GameData::load(const Path& directory) {
        GameData data;
        readGame(directory / "game.json", data);

        // Next files go here, for example:
        // readCampaigns(directory / "campaigns.json", data);

        instance() = std::move(data);
        loaded = true;
    }

    const GameData& GameData::get() {
        if (!loaded)
            throw std::logic_error("GameData::get() called before GameData::load()");

        return instance();
    }

} // namespace takeover
