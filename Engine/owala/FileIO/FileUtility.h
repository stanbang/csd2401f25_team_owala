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
		// Sanitizes and concats the given string (for now just replacing spaces with underscores)
		static string SanitizeFileName(const string& name);
		
		// Read & Write helpers
		static const bool ReadFile(const string& fileName, Document& doc);
		static const bool WriteFileJson(const string& fileName, Document& d);

		// Load window width and height from json file
		Vector2D LoadWindowConfig();

		// Load AnimationInfo from a given filepath
		AnimationInfo LoadAnimationInfo(std::string filepath);

		// Reads SceneAssets file (scenes.json) and assigns its data to doc
		static bool LoadSceneAssets(Document& doc);

		// Save a single SceneAsset to scenes.json
		static bool SaveSceneAsset(const SceneAsset& sceneAsset);

		// Saves all scenes to scenes.json. Unused for now but might be useful in future
		static bool SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets);
	};

	namespace FileIOKeys {
		static constexpr const char* WIN_CONFIG_FILEPATH = "Data/config.json";
		static constexpr const char* PLAYER_CONFIG_FILEPATH = "Data/player.json";
		static constexpr const char* SCENE_ASSETS_FILEPATH = "../../Data/scenes.json";
		static constexpr const char* SCENES_FILEPATH_PREFIX = "../../Data/Scenes/";

		static constexpr const char* WINDOW_W = "window_width";
		static constexpr const char* WINDOW_H = "window_height";

		static constexpr const char* FRAME_COUNT = "frame_count";
		static constexpr const char* FRAME_DURATION = "frame_duration";
		static constexpr const char* IS_LOOPING = "is_looping";
		static constexpr const char* START_FRAME = "start_frame";
	}
} // namespace Framework
