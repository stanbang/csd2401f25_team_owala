/*****************************************************************//**
 * \file   FileUtility.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include <fstream>
#include <iostream>
#include "rapidjson/document.h"
// for reading json file
#include "rapidjson/istreamwrapper.h"
// for writing to json file
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

#include "FileUtility.h"
#include "Vector2D.h"
#include "Components.h"

using namespace std;
using namespace rapidjson;

namespace Framework {
	const string FileIOSystem::WIN_CONFIG_FILEPATH = "Data/config.json";
	const string FileIOSystem::PLAYER_CONFIG_FILEPATH = "Data/player.json";

	Document readFile(const string& fileName) {
		ifstream file(fileName);
		if (!file) throw runtime_error("Could not open file: " + fileName);

		IStreamWrapper isw(file);
		Document d;
		d.ParseStream(isw);

		file.close();
		return d;
	}

	bool writeFileJson(const string& fileName, Document& d) {
		// Write JSON file
		std::ofstream os(fileName);
		if (!os) throw std::runtime_error("Could not open file: " + fileName);

		// Convert Document to JSON string
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		d.Accept(writer);

		// Write to file
		os << buffer.GetString();
		return true;
	}

	Vector2D FileIOSystem::loadWindowConfig() {
		Document d = readFile(WIN_CONFIG_FILEPATH);

		if (!d.HasMember("windowConfig") || !d["windowConfig"].IsObject()) {
			throw runtime_error("Invalid or missing windowConfig in config.json");
		}

		const Value& config = d["windowConfig"];

		if (!config.HasMember("width") || !config["width"].IsInt() ||
			!config.HasMember("height") || !config["height"].IsInt()) {
			throw runtime_error("Invalid or missing data in config.json");
		}

		Vector2D windowSize = { config["width"].GetFloat() ,config["height"].GetFloat() };

		// Debugging output
		cout << "Config data read from config.json:\n"
			 << "Window Size = " << windowSize.x << " x " << windowSize.y
			 << endl;

		return windowSize;
	}

	PlayerStats FileIOSystem::loadPlayerStats() {
		Document d = readFile(PLAYER_CONFIG_FILEPATH); //Read the file name passed in as a path

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			throw runtime_error("Invalid or missing playerStats in player.json");
		}

		const Value& stats = d["playerStats"]; // Extract the playerStats data from the JSON file

		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			throw runtime_error("Invalid or missing properties in player.json");
		}

		const Value& playerX = stats["positionX"];
		const Value& playerY = stats["positionY"];

		std::cout << "Player Position read from player.json:\n"
			<< "Position X: " << playerX.GetFloat() << "\n"
			<< "Position Y: " << playerY.GetFloat() << std::endl;

		Vector2D position{ playerX.GetFloat(), playerY.GetFloat() };

		// Since moveSpeed is a float, convert it to float before storing it in the playerStats object
		PlayerStats playerStats = { stats["moveSpeed"].GetFloat(), Position(position)};

		// Debugging output
		cout << "Player data read from player.json:\n"
			<< "moveSpeed = " << playerStats.moveSpeed
			<< endl;
		return playerStats;
	}

	bool FileIOSystem::writePlayerStats(const PlayerStats& playerStats) {
		Document d = readFile(PLAYER_CONFIG_FILEPATH);

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			throw std::runtime_error("Invalid or missing playerStats in player.json");
		}

		// Modify the JSON data
		Value& stats = d["playerStats"]; // Extract the playerStats object
		
		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			throw runtime_error("Invalid or missing properties in player.json");
		}

		Value& playerX = stats["positionX"];
		Value& playerY = stats["positionY"];

		std::cout << "Updated Player Position\n"
			<< "Position X: " << playerStats.pos.Pos.x << "\n"
			<< "Position Y: " << playerStats.pos.Pos.y << std::endl;

		// Update x and y from your game data
		playerX.SetFloat(playerStats.pos.Pos.x);
		playerY.SetFloat(playerStats.pos.Pos.y);

		return writeFileJson(PLAYER_CONFIG_FILEPATH, d);
	}
}
