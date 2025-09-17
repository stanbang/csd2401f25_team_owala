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
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"

#include "Vector2D.h"
#include "Components.h"
#include "ISystem.h"

namespace Framework {
	using namespace std;
	using namespace rapidjson;

	Document readFile(const string& fileName);
	bool writeFileJson(const string& fileName, Document& d);

	class FileIOSystem {
		static const string WIN_CONFIG_FILEPATH, PLAYER_CONFIG_FILEPATH;

	public:
		Vector2D loadWindowConfig();
		PlayerStats loadPlayerStats();
		bool writePlayerStats(const PlayerStats& playerStats);
	};
} // namespace Framework
