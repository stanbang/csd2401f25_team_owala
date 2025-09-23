/*****************************************************************//**
 * \file   FileUtility.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include <iostream>
#include "rapidjson/document.h"

#include "FileUtility.h"
#include "Vector2D.h"
#include "Components.h"
#include "Logger.h"

namespace Framework {
	const std::string FileIOSystem::WIN_CONFIG_FILEPATH = "Data/config.json";
	const std::string FileIOSystem::PLAYER_CONFIG_FILEPATH = "Data/player.json";

	Vector2D FileIOSystem::LoadWindowConfig() {
		Logger logger;
		std::string logMsg{};
		rapidjson::Document d = ReadFile(WIN_CONFIG_FILEPATH);

		if (!d.HasMember("windowConfig") || !d["windowConfig"].IsObject()) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing windowConfig in config.json");
		}

		const rapidjson::Value& config = d["windowConfig"];

		if (!config.HasMember("width") || !config["width"].IsInt() ||
			!config.HasMember("height") || !config["height"].IsInt()) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing data in config.json");
		}

		Vector2D windowSize = { config["width"].GetFloat() ,config["height"].GetFloat() };

		// Debugging output
		logMsg = format(
			"Config data read from config.json: Window Size = {} x {}",
			windowSize.x, windowSize.y
		);

		logger.Info(DebugType::FileIOSystem, logMsg);

		return windowSize;
	}

	PlayerStats FileIOSystem::LoadPlayerStats() {
		Logger logger;
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
		logMsg = format(
			"Player Position read from player.json: X: {} Y: {}",
			playerX.GetFloat(), playerY.GetFloat()
		);
		logger.Info(DebugType::FileIOSystem, logMsg);

		Vector2D position{ playerX.GetFloat(), playerY.GetFloat() };

		// Since moveSpeed is a float, convert it to float before storing it in the playerStats object
		PlayerStats playerStats = { stats["moveSpeed"].GetFloat(), Position(position)};

		// Debugging output
		logMsg = format(
			"Player data read from player.json: moveSpeed: {}",
			playerStats.moveSpeed
		);
		logger.Info(DebugType::FileIOSystem, logMsg);

		return playerStats;
	}

	bool FileIOSystem::WritePlayerStats(const PlayerStats& playerStats) {
		Logger logger;
		std::string logMsg{};
		rapidjson::Document d = ReadFile(PLAYER_CONFIG_FILEPATH);

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing playerStats in player.json");
		}

		// Modify the JSON data
		rapidjson::Value& stats = d["playerStats"]; // Extract the playerStats object
		
		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing properties in player.json");
		}

		rapidjson::Value& playerX = stats["positionX"];
		rapidjson::Value& playerY = stats["positionY"];

		// Debugging output
		logMsg = format(
			"Updated Player Position: X: {} Y: {}",
			playerStats.pos.Pos.x, playerStats.pos.Pos.y
		);
		logger.Info(DebugType::FileIOSystem, logMsg);

		// Update x and y from your game data
		playerX.SetFloat(playerStats.pos.Pos.x);
		playerY.SetFloat(playerStats.pos.Pos.y);

		return WriteFileJson(PLAYER_CONFIG_FILEPATH, d);
	}
}
