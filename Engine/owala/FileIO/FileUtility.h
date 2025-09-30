/*****************************************************************//**
 * \file   FileUtility.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief
 *		Defines the FileIOSystem class, a utility for reading and writing game
 *		configuration files (e.g., player stats, window settings) using the
 *		RapidJSON library.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
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

#include "Animation.h"
#include "Components.h"
#include "Logger.h"

using namespace std;
using namespace rapidjson;

namespace Framework {
	class FileIOSystem {
		static const string WIN_CONFIG_FILEPATH, PLAYER_CONFIG_FILEPATH;

		static const Document ReadFile(const string& fileName) {
			ifstream file(fileName);
			if (!file) throw GameException(DebugType::FileIOSystem, "Could not open file: " + fileName);

			IStreamWrapper isw(file);
			Document d;
			d.ParseStream(isw);

			file.close();
			return d;
		}

		static const bool WriteFileJson(const string& fileName, Document& d) {
			try {
				// Write JSON file
				std::ofstream os(fileName);
				if (!os) throw GameException(DebugType::FileIOSystem, "Could not open file: " + fileName);
					
				// Convert Document to JSON string
				StringBuffer buffer;
				Writer<StringBuffer> writer(buffer);
				d.Accept(writer);

				// Write to file
				os << buffer.GetString();
				return true;
			}
			catch (const exception& e) {
				throw GameException(DebugType::FileIOSystem, e.what());
			}
		}

	public:
		Vector2D LoadWindowConfig();
		PlayerStats LoadPlayerStats();
		AnimationInfo LoadAnimationInfo(std::string filepath);
		bool WritePlayerStats(const PlayerStats& playerStats);
	};
} // namespace Framework
