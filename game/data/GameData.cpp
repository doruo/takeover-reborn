#include "game/data/GameData.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace takeover {

    namespace {

        GameData& instance() {
            static GameData data;
            return data;
        }

        bool loaded = false;

        /// Parses a JSON file, // comments allowed.
        nlohmann::json readJson(const std::filesystem::path& file) {
            std::ifstream stream(file);

            if (!stream)
                throw std::runtime_error("missing data file " + file.string());

            try {
                return nlohmann::json::parse(stream, nullptr, true, true);
            } catch (const nlohmann::json::exception& error) {
                throw std::runtime_error(file.string() + ": " + error.what());
            }
        }

        // /----- game.json -----/

        void readGame(const std::filesystem::path& file, GameData& data) {
            const nlohmann::json json = readJson(file);

            try {
                // Game
                const nlohmann::json& game = json.at("game");

                data.game.title = game.at("title").get<std::string>();
                data.game.version = game.at("version").get<std::string>();

                // Options
                const nlohmann::json& options = json.at("options");

                data.options.width = options.at("width").get<int>();
                data.options.height = options.at("height").get<int>();
                data.options.resizable = options.at("resizable").get<bool>();
                data.options.hidden = options.at("hidden").get<bool>();
                data.options.vsync = options.at("vsync").get<bool>();

                // Links
                const nlohmann::json& links = json.at("links");

                data.links.armorGames = links.at("armorGames").get<std::string>();
                data.links.a10 = links.at("a10").get<std::string>();
                data.links.iriySoft = links.at("iriySoft").get<std::string>();
                data.links.iriySoftTwitter = links.at("iriySoftTwitter").get<std::string>();
                data.links.iriySoftFacebook = links.at("iriySoftFacebook").get<std::string>();
            } 
            catch (const nlohmann::json::exception& error) {
                throw std::runtime_error(file.string() + ": " + error.what());
            }
        }

    } // namespace

    void GameData::load(const std::filesystem::path& directory) {
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
