#include "FileUtility.h"

namespace Framework {
	const std::string FileIOSystem::WIN_CONFIG_FILEPATH = "Data/config.json";
	const std::string FileIOSystem::PLAYER_CONFIG_FILEPATH = "Data/player.json";

	Vector2D FileIOSystem::LoadWindowConfig() {
		std::string logMsg{};
		rapidjson::Document d = ReadFile(WIN_CONFIG_FILEPATH);

		if (!d.HasMember("windowConfig") || !d["windowConfig"].IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing windowConfig in config.json. Defaulting to 1920x1080" << std::endl;
			return Vector2D(1920.0f, 1080.0f);
		}

		const rapidjson::Value& config = d["windowConfig"];

		if (!config.HasMember("width") || !config["width"].IsInt() ||
			!config.HasMember("height") || !config["height"].IsInt()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing data in config.json. Defaulting to 1920x1080" << std::endl;
			return Vector2D(1920.0f, 1080.0f);
		}

		Vector2D windowSize = { config["width"].GetFloat() ,config["height"].GetFloat() };

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Config data read from config.json: Window Size = " << windowSize.x << " x " << windowSize.y << std::endl;
		return windowSize;
	}

	PlayerStats FileIOSystem::LoadPlayerStats() {
		std::string logMsg{};
		rapidjson::Document d = ReadFile(PLAYER_CONFIG_FILEPATH); //Read the file name passed in as a path

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing playerStats in player.json");
		}

		const rapidjson::Value& stats = d["playerStats"]; // Extract the playerStats data from the JSON file

		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing properties in player.json");
		}

		const rapidjson::Value& playerX = stats["positionX"];
		const rapidjson::Value& playerY = stats["positionY"];

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Player Position read from player.json: X: " << playerX.GetFloat() << " Y: " << playerY.GetFloat() << std::endl;

		Vector2D position{ playerX.GetFloat(), playerY.GetFloat() };

		// Since moveSpeed is a float, convert it to float before storing it in the playerStats object
		PlayerStats playerStats = { stats["moveSpeed"].GetFloat(), Position(position)};

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Player data read from player.json: moveSpeed: " << playerStats.moveSpeed << std::endl;

		return playerStats;
	}

	bool FileIOSystem::WritePlayerStats(const PlayerStats& playerStats) {
		std::string logMsg{};
		rapidjson::Document d = ReadFile(PLAYER_CONFIG_FILEPATH);

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing playerStats in player.json. Unable to write" << std::endl;
			return false;
		}

		// Modify the JSON data
		rapidjson::Value& stats = d["playerStats"]; // Extract the playerStats object
		
		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing properties in player.json. Unable to write" << std::endl;
			return false;
		}

		rapidjson::Value& playerX = stats["positionX"];
		rapidjson::Value& playerY = stats["positionY"];

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Updated Player Position: X: " << playerStats.pos.Pos.x << " Y: " << playerStats.pos.Pos.y << std::endl;

		// Update x and y from your game data
		playerX.SetFloat(playerStats.pos.Pos.x);
		playerY.SetFloat(playerStats.pos.Pos.y);

		return WriteFileJson(PLAYER_CONFIG_FILEPATH, d);
	}
}
