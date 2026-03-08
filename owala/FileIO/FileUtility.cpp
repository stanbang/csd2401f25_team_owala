/*****************************************************************//**
 * \file   FileUtility.cpp
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
#include "pch.h"
#include "FileUtility.h"

#include "GameManager.h"

namespace Framework {
	std::string FileIOSystem::sourceDataDir_;

	std::string FileIOSystem::SanitizeFileName(const std::string& name) {
		std::string sanitized_name = name;
		/*std::transform(sanitized_name.begin(), sanitized_name.end(), sanitized_name.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });*/

			
		std::transform(sanitized_name.begin(), sanitized_name.end(), sanitized_name.begin(),
			[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

		std::replace(sanitized_name.begin(), sanitized_name.end(), ' ', '_');

		return sanitized_name;
	}

	bool FileIOSystem::ReadFile(const string& fileName, Document& doc) {
		ifstream file(fileName);
		if (!file) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Could not open file: " << fileName << std::endl;
			return false;
		}

		IStreamWrapper isw(file);
		doc.ParseStream(isw);

		file.close();
		return true;
	}

	bool FileIOSystem::WriteFileJson(const string& fileName, Document& d, bool writeToRoot) {
		// Write JSON file
		std::ofstream os(fileName);
		if (!os) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Could not open file: " << fileName << std::endl;
			return false;
		}

		// Convert Document to JSON string
		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		d.Accept(writer);

		// Write to local file
		os << buffer.GetString();
		Logger::Get().Info(DebugType::FileIOSystem) << "Successfully wrote to: " << fileName << std::endl;

		if (writeToRoot && !sourceDataDir_.empty()) {
			// Mirror the save back to the source project's Data folder so edits
			// are preserved in source control, not just the build output.
			namespace fs = std::filesystem;
			// fileName is relative like "Data/Scenes/foo.json" — replace the
			// leading "Data" segment with the absolute sourceDataDir_.
			std::string relative = fileName; // e.g. "Data/Scenes/foo.json"
			const std::string dataPrefix = "Data/";
			std::string sourceFilePath;
			if (relative.rfind(dataPrefix, 0) == 0) {
				sourceFilePath = sourceDataDir_ + "/" + relative.substr(dataPrefix.size());
			} else {
				sourceFilePath = sourceDataDir_ + "/" + relative;
			}
			// Ensure the parent directory exists before writing.
			fs::path sourcePath(sourceFilePath);
			if (!fs::exists(sourcePath.parent_path())) {
				fs::create_directories(sourcePath.parent_path());
			}
			std::ofstream rootOs(sourceFilePath);
			if (!rootOs) {
				Logger::Get().Error(DebugType::FileIOSystem)
					<< "Could not write back to source project: " << sourceFilePath << std::endl;
			} else {
				rootOs << buffer.GetString();
				Logger::Get().Info(DebugType::FileIOSystem)
					<< "Mirrored save to source project: " << sourceFilePath << std::endl;
			}
		}

		return true;
	}

	bool FileIOSystem::DeleteFileJson(const string& fileName, bool deleteFromRoot) {
		if (!fs::exists(fileName)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Can't find file to delete: " << fileName << std::endl;
			return false; 
		}

		fs::remove(fileName);
		Logger::Get().Info(DebugType::FileIOSystem) << "Deleted file: " << fileName << std::endl;

		if (deleteFromRoot && !sourceDataDir_.empty()) {
			namespace fs = std::filesystem;
			std::string relative = fileName;
			const std::string dataPrefix = "Data/";
			std::string sourceFilePath;
			if (relative.rfind(dataPrefix, 0) == 0) {
				sourceFilePath = sourceDataDir_ + "/" + relative.substr(dataPrefix.size());
			} else {
				sourceFilePath = sourceDataDir_ + "/" + relative;
			}
			if (fs::exists(sourceFilePath)) {
				fs::remove(sourceFilePath);
				Logger::Get().Info(DebugType::FileIOSystem)
					<< "Deleted from source project: " << sourceFilePath << std::endl;
			} else {
				Logger::Get().Error(DebugType::FileIOSystem)
					<< "Can't find file to delete in source project: " << sourceFilePath << std::endl;
			}
		}

		return true;
	}

	WindowConfig FileIOSystem::LoadWindowConfig() {
		rapidjson::Document d;
		Vector2D windowSize = Vector2D(1920.0f, 1080.0f);
		bool fullScreen = true;

		//Read the file name passed in as a path
		if (!ReadFile(FileIOKeys::WIN_CONFIG_FILEPATH, d) || !d.IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read or parse config.json. Defaulting to 1920x1080" << std::endl;
			return WindowConfig(windowSize, fullScreen);
		}

		// Check that extracted data has the expected properties
		if (!d.HasMember(FileIOKeys::WINDOW_W) || !d[FileIOKeys::WINDOW_W].IsInt() ||
			!d.HasMember(FileIOKeys::WINDOW_H) || !d[FileIOKeys::WINDOW_H].IsInt()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing data in config.json. Defaulting to 1920x1080" << std::endl;
			return WindowConfig(windowSize, fullScreen);
		}

		windowSize = { d[FileIOKeys::WINDOW_W].GetFloat() ,d[FileIOKeys::WINDOW_H].GetFloat() };
		Logger::Get().Info(DebugType::FileIOSystem) << "Config data read from config.json: Window Size = " << windowSize.x << " x " << windowSize.y << std::endl;

		if (d.HasMember(FileIOKeys::WINDOW_FULLSCREEN) && d[FileIOKeys::WINDOW_FULLSCREEN].IsBool()) {
			fullScreen = d[FileIOKeys::WINDOW_FULLSCREEN].GetBool();
			Logger::Get().Info(DebugType::FileIOSystem) << "Config data read from config.json: Window fullscreen = " << fullScreen << std::endl;
		}

		// Debugging output
		return WindowConfig(windowSize, fullScreen);
	}
	void FileIOSystem::LoadPlayerStats(PlayerStats& stats) {
		rapidjson::Document d;

		if (!ReadFile(FileIOKeys::PLAYER_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read file: " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
			return;
		}

		// Helper lambda to safely read floats without crashing
		auto SafeReadFloat = [&](const char* key, float& outValue) {
			if (d.HasMember(key) && d[key].IsNumber()) {
				outValue = d[key].GetFloat();
			}
			else {
				Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing '" << key << "' in " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
			}
			};

		SafeReadFloat(FileIOKeys::PLAYER_MV_SPEED, stats.moveSpeed);
		SafeReadFloat(FileIOKeys::PLAYER_JUMP_HEIGHT, stats.jumpHeight);
		SafeReadFloat(FileIOKeys::PLAYER_INVULNERABILITY_DURATION, stats.invulnerabilityDuration);
		SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_DURATION, stats.knockbackDuration);
		SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_FORCE_X, stats.knockbackForceX);
		SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_FORCE_Y, stats.knockbackForceY);

		Logger::Get().Info(DebugType::FileIOSystem) << "Player data read successfully." << std::endl;
	}

	void FileIOSystem::SaveWorldConfig() {
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		/* Physics Config Serialization */
		rapidjson::Value physicsObj(rapidjson::kObjectType);
		PhysicsSystem* physicsSystem = GameManager::GetInstance().GetSystems().physics_;

		if (physicsSystem) {
			physicsObj.AddMember(rapidjson::StringRef(FileIOKeys::GBL_GRAVITYX), physicsSystem->gravity_.x, alloc);
			physicsObj.AddMember(rapidjson::StringRef(FileIOKeys::GBL_GRAVITYY), physicsSystem->gravity_.y, alloc);
		}

		doc.AddMember(rapidjson::StringRef(FileIOKeys::GBL_PHYSICSCAT), physicsObj, alloc);

		/* Performance Config Serialization */
		rapidjson::Value performanceObj(rapidjson::kObjectType);
		Graphics* graphicsSystem = GameManager::GetInstance().GetSystems().gfx_;

		performanceObj.AddMember(rapidjson::StringRef(FileIOKeys::GBL_ENABLEFPSLIMIT), graphicsSystem->IsFPSCapEnabled(), alloc);
		performanceObj.AddMember(rapidjson::StringRef(FileIOKeys::GBL_MAXFPS), graphicsSystem->GetFPSCap(), alloc);

		doc.AddMember(rapidjson::StringRef(FileIOKeys::GBL_PERFORMANCECAT), performanceObj, alloc);

		if (WriteFileJson(FileIOKeys::WORLD_CONFIG_FILEPATH, doc)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "World Config Saved!" << std::endl;
		}
	}

	void FileIOSystem::LoadWorldConfig() {
		rapidjson::Document doc;
		//Read the file name passed in as a path
		if (!ReadFile(FileIOKeys::WORLD_CONFIG_FILEPATH, doc)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Unable to read file: " << FileIOKeys::WORLD_CONFIG_FILEPATH << std::endl;
			return;
		}

		/* Physics Config Deserialization */
		if (doc.HasMember(FileIOKeys::GBL_PHYSICSCAT) && doc[FileIOKeys::GBL_PHYSICSCAT].IsObject()) {
			const rapidjson::Value& physicsObj = doc[FileIOKeys::GBL_PHYSICSCAT];
			PhysicsSystem* physicsSystem = GameManager::GetInstance().GetSystems().physics_;

			if (physicsSystem) {
				if (physicsObj.HasMember(FileIOKeys::GBL_GRAVITYX) && physicsObj[FileIOKeys::GBL_GRAVITYX].IsFloat()) {
					physicsSystem->gravity_.x = physicsObj[FileIOKeys::GBL_GRAVITYX].GetFloat();
				}
				if (physicsObj.HasMember(FileIOKeys::GBL_GRAVITYY) && physicsObj[FileIOKeys::GBL_GRAVITYY].IsFloat()) {
					physicsSystem->gravity_.y = physicsObj[FileIOKeys::GBL_GRAVITYY].GetFloat();
				}
				Logger::Get().Info(DebugType::FileIOSystem) << "Physics Configuration Loaded" << std::endl;
			}
		}

		/* Performance Config Deserialization */
		if (doc.HasMember(FileIOKeys::GBL_PERFORMANCECAT) && doc[FileIOKeys::GBL_PERFORMANCECAT].IsObject()) {
			const rapidjson::Value& performanceObj = doc[FileIOKeys::GBL_PERFORMANCECAT];
			Graphics* graphicsSystem = GameManager::GetInstance().GetSystems().gfx_;
			
			if (graphicsSystem) {
				if (performanceObj.HasMember(FileIOKeys::GBL_ENABLEFPSLIMIT) && performanceObj[FileIOKeys::GBL_ENABLEFPSLIMIT].IsBool()) {
					graphicsSystem->ToggleFPSCap(performanceObj[FileIOKeys::GBL_ENABLEFPSLIMIT].GetBool());
				}
				if (performanceObj.HasMember(FileIOKeys::GBL_MAXFPS) && performanceObj[FileIOKeys::GBL_MAXFPS].IsFloat()) {
					graphicsSystem->SetFPSCap(performanceObj[FileIOKeys::GBL_MAXFPS].GetFloat());
				}
				Logger::Get().Info(DebugType::FileIOSystem) << "Performance Configuration Loaded" << std::endl;
			}
		}
	}

	AnimationInfo FileIOSystem::LoadAnimationInfo(std::string filepath) {
		rapidjson::Document d;
		ReadFile(filepath, d); //Read the file name passed in as a path

		// Check if the JSON file is an object
		if (!d.IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing animation data in " << filepath << std::endl;
			return AnimationInfo{};
		}

		// Check if animation members exists
		if (!d.HasMember(FileIOKeys::FRAME_COUNT) || !d.HasMember(FileIOKeys::FRAME_DURATION) || !d.HasMember(FileIOKeys::IS_LOOPING) || !d.HasMember(FileIOKeys::START_FRAME)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing animation data in " << filepath << std::endl;
			return AnimationInfo{};
		}

		const rapidjson::Value& frameCount = d[FileIOKeys::FRAME_COUNT];
		const rapidjson::Value& frameDuration = d[FileIOKeys::FRAME_DURATION];
		const rapidjson::Value& isLooping = d[FileIOKeys::IS_LOOPING];
		const rapidjson::Value& startFrame = d[FileIOKeys::START_FRAME];

		// Debugging output
		//Logger::Get().Info(DebugType::FileIOSystem) << "Animation Data read from " << filepath << ": frameCount: " << frameCount.GetInt() << ", frameDuration : " << frameDuration.GetFloat() << ", isLooping : " << isLooping.GetBool() << ", startFrame : " << startFrame.GetInt() << std::endl;

		//Create AnimationComponent from data
		AnimationInfo anim{ frameCount.GetInt(), frameDuration.GetFloat(), isLooping.GetBool(), startFrame.GetInt()};

		// Push relevant spritesheets into animation info component
		if (d.HasMember("spritesheets") && d["spritesheets"].IsArray()) {
			const rapidjson::Value& sheetsJson = d["spritesheets"];
			for (rapidjson::SizeType i = 0; i < sheetsJson.Size(); ++i) {
				const rapidjson::Value& sheet = sheetsJson[i];
				if (sheet.HasMember("texture") && sheet.HasMember("count")) {
					anim.sheets.push_back({
						sheet["texture"].GetString(),
						sheet["count"].GetInt()
						});
				}
			}
		}

		// Map frame events if they exist
		if (d.HasMember("frameEvents") && d["frameEvents"].IsObject()) {
			const rapidjson::Value& eventsJson = d["frameEvents"];
			for (auto it = eventsJson.MemberBegin(); it != eventsJson.MemberEnd(); ++it) {
				// RapidJSON object keys are always strings, convert back to int
				int frameIdx = std::stoi(it->name.GetString());
				std::string evtName = it->value.GetString();
				anim.frameEvents[frameIdx] = evtName;
			}
		}

		return anim;
	}

	void FileIOSystem::LoadAudioFileNames(std::unordered_map<std::string, std::vector<std::string>>& audioFilesMap) {
		rapidjson::Document d;
		ReadFile(FileIOKeys::AUDIO_NAMES_FILEPATH, d); //Read the file name passed in as a path

		// Check if the JSON file is an object
		if (!d.IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing audio data in " << FileIOKeys::AUDIO_NAMES_FILEPATH << std::endl;
			return;
		}

		for (rapidjson::GenericMemberIterator itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr) {
			std::string key = itr->name.GetString();
			
			if (itr->value.IsArray()) {
				for (rapidjson::Value& v : itr->value.GetArray()) {
					if (v.IsString()) {
						std::string filePath = v.GetString();
						audioFilesMap[key].push_back(filePath);
					}
				}
			}
		}
	}

	bool FileIOSystem::LoadSceneAssets(rapidjson::Document& doc) {
		rapidjson::Document d;
		if (!ReadFile(FileIOKeys::SCENE_ASSETS_FILEPATH, d) || !d.IsArray()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid data in: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			return false;
		}

		doc.Swap(d);
		return true;
	}

	bool FileIOSystem::SaveSceneAsset(const SceneAsset& sceneAsset) {
		try {
			Logger::Get().Info(DebugType::FileIOSystem) << "Saving scene asset <" << sceneAsset.name << "> to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			rapidjson::Document d;
			if (!ReadFile(FileIOKeys::SCENE_ASSETS_FILEPATH, d) || !d.IsArray()) {
				d.SetArray();
			}
			
			rapidjson::Document::AllocatorType& alloc = d.GetAllocator();

			bool sceneAssetExists = false;
			for (rapidjson::SizeType i = 0; i < d.GetArray().Size(); ++i) {
				rapidjson::Value& val = d.GetArray()[i];
			
				if (!val.IsObject() || !val.HasMember(Serialization::SCENE_ASSET_NAME) || !val.HasMember(Serialization::SCENE_ASSET_PATH)) {
					Logger::Get().Error(DebugType::FileIOSystem) << "Invalid asset structure encountered in: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
					return false;
				}

				if (val[Serialization::SCENE_ASSET_NAME].GetString() == sceneAsset.name) {
					sceneAssetExists = true;
					val[Serialization::SCENE_ASSET_PATH].SetString(sceneAsset.path.c_str(), alloc);
					break;
				}
			}

			if (!sceneAssetExists) {
				rapidjson::Value asset(kObjectType);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_NAME), rapidjson::Value(sceneAsset.name.c_str(), alloc).Move(), alloc);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_PATH), rapidjson::Value(sceneAsset.path.c_str(), alloc).Move(), alloc);
				d.PushBack(asset, alloc);
			}

			// Write to Data folder
			if (WriteFileJson(FileIOKeys::SCENE_ASSETS_FILEPATH, d)) {
				Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved scene asset <" << sceneAsset.name << "> to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			}
			else { throw std::runtime_error("Failed to save scene asset."); }
			
			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save scene asset <" << sceneAsset.name << "> to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			return false;
		}
	}

	bool FileIOSystem::SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets) {
		try {
			Logger::Get().Info(DebugType::FileIOSystem) << "Saving all " << allAssets.size() << " scene assets to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;

			rapidjson::Document d;
			d.SetArray();
			rapidjson::Document::AllocatorType& alloc = d.GetAllocator();

			for (const auto& [id, sceneAsset] : allAssets) {
				rapidjson::Value asset(kObjectType);

				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_NAME), rapidjson::Value(sceneAsset.name.c_str(), alloc).Move(), alloc);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_PATH), rapidjson::Value(sceneAsset.path.c_str(), alloc).Move(), alloc);

				d.PushBack(asset, alloc);
			}

			// Write to Data folder
			if (WriteFileJson(FileIOKeys::SCENE_ASSETS_FILEPATH, d)) {
				Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved all scene assets to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			}
			else { throw std::runtime_error("Failed to save all scene assets."); }
			
			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save all scene assets." << std::endl;
			return false;
		}
	}

	GameConfig FileIOSystem::LoadGameConfig() {
		GameConfig config;
		rapidjson::Document doc;

		if (!FileIOSystem::ReadFile(FileIOKeys::GAME_CONFIG_FILEPATH, doc) || !doc.IsObject()) {
			Logger::Get().Info(DebugType::FileIOSystem)
				<< "GameConfig.json not found or invalid - using empty config." << std::endl;
			return config;
		}

		if (doc.HasMember(FileIOKeys::GC_STARTUP_SCENE) && doc[FileIOKeys::GC_STARTUP_SCENE].IsString())
			config.startupScene = doc[FileIOKeys::GC_STARTUP_SCENE].GetString();

		if (doc.HasMember(FileIOKeys::GC_LOADING_SCENE) && doc[FileIOKeys::GC_LOADING_SCENE].IsString())
			config.loadingScene = doc[FileIOKeys::GC_LOADING_SCENE].GetString();

		if (doc.HasMember(FileIOKeys::GC_OVERLAY_SCENES) && doc[FileIOKeys::GC_OVERLAY_SCENES].IsArray())
			for (const auto& v : doc[FileIOKeys::GC_OVERLAY_SCENES].GetArray())
				if (v.IsString()) config.overlayScenes.push_back(v.GetString());

		if (doc.HasMember(FileIOKeys::GC_MENU_SCENES) && doc[FileIOKeys::GC_MENU_SCENES].IsArray())
			for (const auto& v : doc[FileIOKeys::GC_MENU_SCENES].GetArray())
				if (v.IsString()) config.menuScenes.push_back(v.GetString());

		Logger::Get().Info(DebugType::FileIOSystem)
			<< "Loaded GameConfig.json: startup='" << config.startupScene
			<< "', loading='" << config.loadingScene << "'" << std::endl;
		return config;
	}

	bool FileIOSystem::SaveGameConfig(const GameConfig& config) {
		rapidjson::Document doc;
		doc.SetObject();
		auto& alloc = doc.GetAllocator();

		doc.AddMember(rapidjson::StringRef(FileIOKeys::GC_STARTUP_SCENE),
			rapidjson::Value(config.startupScene.c_str(), alloc), alloc);

		doc.AddMember(rapidjson::StringRef(FileIOKeys::GC_LOADING_SCENE),
			rapidjson::Value(config.loadingScene.c_str(), alloc), alloc);

		rapidjson::Value overlayArr(rapidjson::kArrayType);
		for (const auto& s : config.overlayScenes)
			overlayArr.PushBack(rapidjson::Value(s.c_str(), alloc), alloc);
		doc.AddMember(rapidjson::StringRef(FileIOKeys::GC_OVERLAY_SCENES), overlayArr, alloc);

		rapidjson::Value menuArr(rapidjson::kArrayType);
		for (const auto& s : config.menuScenes)
			menuArr.PushBack(rapidjson::Value(s.c_str(), alloc), alloc);
		doc.AddMember(rapidjson::StringRef(FileIOKeys::GC_MENU_SCENES), menuArr, alloc);

		if (WriteFileJson(FileIOKeys::GAME_CONFIG_FILEPATH, doc)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Saved GameConfig.json." << std::endl;
			return true;
		}
		Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save GameConfig.json." << std::endl;
		return false;
	}
}
