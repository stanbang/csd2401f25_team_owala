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

#include "GameConfig.h"
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
		static bool ReadFile(const string& fileName, Document& doc);

		// WriteFileJson and DeleteFileJson return true as long as the local Debug file was modified.
		// Any issues modifying the file in root Data folder will be logged but will not return false
		static bool WriteFileJson(const string& fileName, Document& d, bool writeToRoot = true);
		static bool DeleteFileJson(const string& fileName, bool deleteFromRoot = true);

		// Load window width and height from json file
		static WindowConfig LoadWindowConfig();

		// Saves world config variables to world config json file
		static void SaveWorldConfig();

		// Load world config variables from world config json file
		static void LoadWorldConfig();

		// Load AnimationInfo from a given filepath
		static AnimationInfo LoadAnimationInfo(std::string filepath);

		// Load audio file names from audio json file
		static void LoadAudioFileNames(std::unordered_map<std::string, std::vector<std::string>>& audioFilesMap);

		// Load animation event -> audio mappings from game data (animationEventAudio.json)
		static void LoadAnimationEventAudio(std::unordered_map<std::string, AnimationEventAudioConfig>& outMap);

		// Reads SceneAssets file (scenes.json) and assigns its data to doc
		static bool LoadSceneAssets(Document& doc);

		// Save a single SceneAsset to scenes.json
		static bool SaveSceneAsset(const SceneAsset& sceneAsset);

		// Saves all scenes to scenes.json. Unused for now but might be useful in future
		static bool SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets);

		// Load scene categorization config from Data/GameConfig.json.
		// Returns a default-constructed GameConfig if the file is missing or malformed.
		static GameConfig LoadGameConfig();

		// Save scene categorization config to Data/GameConfig.json.
		static bool SaveGameConfig(const GameConfig& config);

		// Set the source project data and assets directory for write-back (e.g. "F:/MyGame/Data").
		// When set, WriteFileJson/DeleteFileJson mirror saves back to this folder so
		// edits are preserved in the source project, not just the build output.
		static void SetSourceDataDir(const std::string& dir) { sourceDataDir_ = dir; }
		static const std::string& GetSourceDataDir() { return sourceDataDir_; }

		static void SetSourceAssetsDir(const std::string& dir) { sourceAssetsDir_ = dir; }
		static const std::string& GetSourceAssetsDir() { return sourceAssetsDir_; }

	private:
		static std::string sourceDataDir_; // absolute path to active game's source Data/ folder
		static std::string sourceAssetsDir_; // absolute path to active game's source Assets/ folder
	};

	namespace FileIOKeys {
		// Filepath constants
		static constexpr const char* ROOT_FOLDER_PREFIX = "../../";
		//static constexpr const char* ROOT_DATA_FOLDER = "../../Data";
		//static constexpr const char* ROOT_ASSETS_FOLDER = "../../Assets";

		static constexpr const char* WIN_CONFIG_FILEPATH = "Data/config.json";
		static constexpr const char* PLAYER_CONFIG_FILEPATH = "Data/player.json";
#ifdef BUILD_EDITOR
		static constexpr const char* EDITOR_CFG_PATH = "Data/Config/editorConfig.json";
#endif
		static constexpr const char* WORLD_CONFIG_FILEPATH = "Data/Config/worldConfig.json";
		static constexpr const char* AUDIO_NAMES_FILEPATH       = "Data/audioFiles.json";
		static constexpr const char* ANIM_EVENT_AUDIO_FILEPATH   = "Data/animationEventAudio.json";

		static constexpr const char* SCENE_ASSETS_FILEPATH = "Data/scenes.json";
		static constexpr const char* SCENES_FILEPATH_PREFIX = "Data/Scenes/";
		static constexpr const char* GAME_CONFIG_FILEPATH = "Data/GameConfig.json";

		// layer manager Json Key
		static constexpr const char* LAYER_CONFIG_FILEPATH = "Data/LayerConfig.json";

		// GameConfig JSON keys
		static constexpr const char* GC_STARTUP_SCENE  = "startupScene";
		static constexpr const char* GC_LOADING_SCENE  = "loadingScene";
		static constexpr const char* GC_OVERLAY_SCENES = "overlayScenes";
		static constexpr const char* GC_MENU_SCENES    = "menuScenes";

#ifdef BUILD_EDITOR
		/* Editor-Only JSON Keys */
		static constexpr const char* EDITOR_HISTORY_STACK = "MaxHistoryStack";
#endif

		/* Globals JSON Keys */
		static constexpr const char* GBL_PHYSICSCAT = "Physics2D";
		static constexpr const char* GBL_GRAVITYX = "DefaultGravityX";
		static constexpr const char* GBL_GRAVITYY = "DefaultGravityY";
		static constexpr const char* GBL_PERFORMANCECAT = "Performance";
		static constexpr const char* GBL_ENABLEFPSLIMIT = "bEnableMaxFPS";
		static constexpr const char* GBL_MAXFPS = "MaxFPS";
		static constexpr const char* GBL_GAMESPEED = "GameSpeed";

		// Window config Json keys
		static constexpr const char* WINDOW_W = "window_width";
		static constexpr const char* WINDOW_H = "window_height";
		static constexpr const char* WINDOW_FULLSCREEN = "full_screen";

		// Player config Json keys
		static constexpr const char* PLAYER_MV_SPEED = "move_speed";
		static constexpr const char* PLAYER_JUMP_HEIGHT = "jump_height";
		static constexpr const char* PLAYER_INVULNERABILITY_DURATION = "invulnerability_duration";
		static constexpr const char* PLAYER_KNOCKBACK_DURATION = "knockback_duration";
		static constexpr const char* PLAYER_KNOCKBACK_FORCE_X = "knockback_force_x";
		static constexpr const char* PLAYER_KNOCKBACK_FORCE_Y = "knockback_force_y";

		// World config Json keys
		static constexpr const char* WORLD_GRAVITY = "gravity";

		// Animation Json keys
		static constexpr const char* FRAME_COUNT = "frame_count";
		static constexpr const char* FRAME_DURATION = "frame_duration";
		static constexpr const char* IS_LOOPING = "is_looping";
		static constexpr const char* START_FRAME = "start_frame";
	}
} // namespace Framework
