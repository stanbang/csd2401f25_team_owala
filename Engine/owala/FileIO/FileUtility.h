/*****************************************************************//**
 * \file   FileUtility.h
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/
#pragma once
#include "rapidjson/document.h"
// for reading json file
#include "rapidjson/istreamwrapper.h"
// for writing to json file
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/filewritestream.h"
#include <iostream>
#include <fstream>

#include "Vector2D.h"
#include "Components.h"
#include "ISystem.h"

namespace Framework {
	using namespace std;
	using namespace rapidjson;

	static Document readFile(const string& fileName) {
		ifstream file(fileName);
		if (!file) throw runtime_error("Could not open file: " + fileName);

		IStreamWrapper isw(file);
		Document d;
		d.ParseStream(isw);

		file.close();
		return d;
	}
	static bool writeFileJson(const string& fileName, Document& d) {
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

	bool writeFileJson(const string& fileName, Document& d);

	class FileIOSystem {
		static const string WIN_CONFIG_FILEPATH, PLAYER_CONFIG_FILEPATH;

	public:
		Vector2D loadWindowConfig();
		PlayerStats loadPlayerStats();
		bool writePlayerStats(const PlayerStats& playerStats);
	};
} // namespace Framework
