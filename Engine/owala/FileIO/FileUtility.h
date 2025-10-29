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

#include "SceneManager.h"
#include "Animation.h"
#include "Components.h"
#include "Logger.h"
#include "SerializationKeys.h"

using namespace std;
using namespace rapidjson;

namespace Framework {
	class FileIOSystem {
	public:
		static const string WIN_CONFIG_FILEPATH, PLAYER_CONFIG_FILEPATH, SCENE_ASSETS_FILEPATH, SCENES_FILEPATH_PREFIX;

		static string SanitizeFileName(const string& name);

		static const bool ReadFile(const string& fileName, Document& doc);
		static const bool WriteFileJson(const string& fileName, Document& d);

		Vector2D LoadWindowConfig();
		PlayerStats LoadPlayerStats();
		AnimationInfo LoadAnimationInfo(std::string filepath);

		static bool LoadSceneAssets(Document& doc);
		static bool SaveSceneAsset(const SceneAsset& sceneAsset);
		static bool SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets);
	};
} // namespace Framework
