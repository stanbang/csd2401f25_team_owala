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
#include "rapidjson/istreamwrapper.h"

#include "FileUtility.h"
#include "Vector2D.h"
#include "Components.h"

using namespace std;
using namespace rapidjson;

namespace Framework {
	Document readFile(const string& fileName) {

		ifstream file(fileName);
		if (!file) throw runtime_error("Could not open file: " + fileName);

		IStreamWrapper isw(file);
		Document d;
		d.ParseStream(isw);
		return d;
	}

	Vector2D FileIOSystem::loadWindowConfig() {
		Document d = readFile("Data/config.json");

		if (!d.HasMember("windowConfig") || !d["windowConfig"].IsObject()) {
			throw runtime_error("Invalid or missing windowConfig in config.json");
		}

		const Value& config = d["windowConfig"];

		// set default 800 by 600 in case read file fail
		float windowW{ 800.0f }, windowH{ 600.0f };

		if (!config.HasMember("width") || !config["width"].IsInt() ||
			!config.HasMember("height") || !config["height"].IsInt()) {
			throw runtime_error("Invalid or missing data in config.json");
		}

		windowW = static_cast<float>(config["width"].GetInt());
		windowH = static_cast<float>(config["height"].GetInt());

		// Debugging output
		cout << "Config data read from config.json:\n"
			 << "Window Size = " << windowW << " x " << windowH
			 << std::endl;

		return Vector2D {windowW, windowH};
	}

	PlayerStats FileIOSystem::loadPlayerStats() {
		Document d = readFile("Data/player.json"); //Read the file name passed in as a path

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			throw runtime_error("Invalid or missing playerStats in playerStats.json");
		}

		const Value& stats = d["playerStats"]; // Extract the playerStats data from the JSON file

		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed")) {
			throw runtime_error("Invalid or missing moveSpeed in playerStats.json");
		}

		// Since moveSpeed is a float, convert it to float before storing it in the playerStats object
		PlayerStats playerStats = { stats["moveSpeed"].GetFloat() };

		// Debugging output
		cout << "Player data read from player.json:\n"
			<< "moveSpeed = " << playerStats.moveSpeed
			<< std::endl;
		return playerStats;
	}
}
