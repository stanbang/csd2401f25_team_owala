/*****************************************************************//**
 * \file   FileUtility.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

#include "FileUtility.h"
#include "Vector2D.h"

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
		Document d = readFile("../Data/config.json");

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

		return Vector2D {windowW, windowH};
	}
}
