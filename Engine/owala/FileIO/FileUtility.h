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
	struct WindowConfig {
		Vector2D windowSize = { 1920.f, 1080.f };
		bool fullScreen = true;
	};

	class FileIOSystem {
	public:
		// Sanitizes and concats the given string (for now just replacing spaces with underscores)
		static string SanitizeFileName(const string& name);
		
		// Read & Write helpers
		static const bool ReadFile(const string& fileName, Document& doc);
		static const bool WriteFileJson(const string& fileName, Document& d);

		// Load window width and height from json file
		static WindowConfig LoadWindowConfig();

		// Load PlayerStats from player config json file
		static void LoadPlayerStats(float& moveSpeed, float& jumpHeight);

		// Load world config variables from world config json file
		static void LoadWorldConfig(float& gravity);

		// Load AnimationInfo from a given filepath
		static AnimationInfo LoadAnimationInfo(std::string filepath);

		// Reads SceneAssets file (scenes.json) and assigns its data to doc
		static bool LoadSceneAssets(Document& doc);

		// Save a single SceneAsset to scenes.json
		static bool SaveSceneAsset(const SceneAsset& sceneAsset);

		// Saves all scenes to scenes.json. Unused for now but might be useful in future
		static bool SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets);
	};

	namespace FileIOKeys {
		// Filepath constants
		static constexpr const char* ROOT_FOLDER_PREFIX = "../../";
		static constexpr const char* ROOT_DATA_FOLDER = "../../Data";
		static constexpr const char* ROOT_ASSETS_FOLDER = "../../Assets";

		static constexpr const char* WIN_CONFIG_FILEPATH = "Data/config.json";
		static constexpr const char* PLAYER_CONFIG_FILEPATH = "Data/player.json";
		static constexpr const char* WORLD_CONFIG_FILEPATH = "Data/world.json";

		static constexpr const char* SCENE_ASSETS_FILEPATH = "Data/scenes.json";
		static constexpr const char* SCENES_FILEPATH_PREFIX = "Data/Scenes/";


		// Window config Json keys
		static constexpr const char* WINDOW_W = "window_width";
		static constexpr const char* WINDOW_H = "window_height";
		static constexpr const char* WINDOW_FULLSCREEN = "full_screen";

		// Player config Json keys
		static constexpr const char* PLAYER_MV_SPEED = "move_speed";
		static constexpr const char* PLAYER_JUMP_HEIGHT = "jump_height";

		// World config Json keys
		static constexpr const char* WORLD_GRAVITY = "gravity";

		// Animation Json keys
		static constexpr const char* FRAME_COUNT = "frame_count";
		static constexpr const char* FRAME_DURATION = "frame_duration";
		static constexpr const char* IS_LOOPING = "is_looping";
		static constexpr const char* START_FRAME = "start_frame";
	}
} // namespace Framework
