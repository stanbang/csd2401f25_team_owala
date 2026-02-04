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

namespace Framework {
	std::string FileIOSystem::SanitizeFileName(const std::string& name) {
		std::string sanitized_name = name;
		/*std::transform(sanitized_name.begin(), sanitized_name.end(), sanitized_name.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });*/

			
		std::transform(sanitized_name.begin(), sanitized_name.end(), sanitized_name.begin(),
			[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

		std::replace(sanitized_name.begin(), sanitized_name.end(), ' ', '_');

		return sanitized_name;
	}

	const bool FileIOSystem::ReadFile(const string& fileName, Document& doc) {
		try {
			ifstream file(fileName);
			if (!file) Logger::Get().Error(DebugType::FileIOSystem) << "Could not open file: " << fileName << std::endl;

			IStreamWrapper isw(file);
			doc.ParseStream(isw);

			file.close();
			return true;
		}
		catch (const exception& e) {
			throw GameException(DebugType::FileIOSystem, e.what());
		}
	}

	const bool FileIOSystem::WriteFileJson(const string& fileName, Document& d) {
		try {
			// Write JSON file
			std::ofstream os(fileName);
			if (!os) Logger::Get().Error(DebugType::FileIOSystem) << "Could not open file: " << fileName << std::endl;

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

	WindowConfig FileIOSystem::LoadWindowConfig() {
		rapidjson::Document d;
		Vector2D windowSize = Vector2D(1920.0f, 1080.0f);
		bool fullScreen = true;

		//Read the file name passed in as a path
		if (!ReadFile(FileIOKeys::WIN_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read config.json. Defaulting to 1920x1080" << std::endl;
			windowSize = Vector2D(1920.0f, 1080.0f);
		}

		// Check if the JSON file is an object
		if (!d.IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing data in config.json. Defaulting to 1920x1080" << std::endl;
			windowSize = Vector2D(1920.0f, 1080.0f);
		}

		// Check that extracted data has the expected properties
		if (!d.HasMember(FileIOKeys::WINDOW_W) || !d[FileIOKeys::WINDOW_W].IsInt() ||
			!d.HasMember(FileIOKeys::WINDOW_H) || !d[FileIOKeys::WINDOW_H].IsInt()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing data in config.json. Defaulting to 1920x1080" << std::endl;
			windowSize = Vector2D(1920.0f, 1080.0f);
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

	void FileIOSystem::LoadPlayerStats(float& moveSpeed, float& jumpHeight) {
		rapidjson::Document d;
		//Read the file name passed in as a path
		if (!ReadFile(FileIOKeys::PLAYER_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read file: " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
		}

		// Check if move_speed member exists && is float
		if (!(d.HasMember(FileIOKeys::PLAYER_MV_SPEED) && d[FileIOKeys::PLAYER_MV_SPEED].IsFloat())) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Invalid or missing move_speed in " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
		}
		// Check if jump_height member exists && is float
		if (!(d.HasMember(FileIOKeys::PLAYER_JUMP_HEIGHT)) && d[FileIOKeys::PLAYER_JUMP_HEIGHT].IsFloat()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing jump_height in " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
		}

		const rapidjson::Value& mvSpeed = d[FileIOKeys::PLAYER_MV_SPEED];
		const rapidjson::Value& jmpHeight = d[FileIOKeys::PLAYER_JUMP_HEIGHT];

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Player data read from " << FileIOKeys::PLAYER_CONFIG_FILEPATH 
			<< ", moveSpeed: " << mvSpeed.GetFloat() << " jumpHeight: " << jmpHeight.GetFloat() << std::endl;

		// Set variables
		moveSpeed = mvSpeed.GetFloat();
		jumpHeight = jmpHeight.GetFloat();
	}

	void FileIOSystem::LoadWorldConfig(float& gravity) {
		rapidjson::Document d;
		//Read the file name passed in as a path
		if (!ReadFile(FileIOKeys::WORLD_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read file: " << FileIOKeys::WORLD_CONFIG_FILEPATH << std::endl;
		}

		// Check if gravity member exists && is float
		if (!(d.HasMember(FileIOKeys::WORLD_GRAVITY) && d[FileIOKeys::WORLD_GRAVITY].IsFloat())) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Invalid or missing gravity in " << FileIOKeys::WORLD_CONFIG_FILEPATH << std::endl;
		}

		const rapidjson::Value& gravityData = d[FileIOKeys::WORLD_GRAVITY];

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "World gravity read from " << FileIOKeys::WORLD_CONFIG_FILEPATH << ": " << gravityData.GetFloat() << std::endl;

		// Set variables
		gravity = gravityData.GetFloat();
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
		Logger::Get().Info(DebugType::FileIOSystem) << "Animation Data read from " << filepath << ": frameCount: " << frameCount.GetInt() << ", frameDuration : " << frameDuration.GetFloat() << ", isLooping : " << isLooping.GetBool() << ", startFrame : " << startFrame.GetInt() << std::endl;

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

			// Write to local Data folder
			WriteFileJson(FileIOKeys::SCENE_ASSETS_FILEPATH, d);
			Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved scene asset <" << sceneAsset.name << "> to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			
			// Check if root Data folder exists and write to it as well
			if (std::filesystem::exists(FileIOKeys::ROOT_DATA_FOLDER)) {
				std::string rootFilePath = std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + FileIOKeys::SCENE_ASSETS_FILEPATH;
				WriteFileJson(rootFilePath, d);
				Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved scene asset <" << sceneAsset.name << "> to: " << rootFilePath << std::endl;
			}
			
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

			// Write to local Data folder
			WriteFileJson(FileIOKeys::SCENE_ASSETS_FILEPATH, d);
			Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved all scene assets to: " << FileIOKeys::SCENE_ASSETS_FILEPATH << std::endl;
			
			// Check if root Data folder exists and write to it as well
			if (std::filesystem::exists(FileIOKeys::ROOT_DATA_FOLDER)) {
				std::string rootFilePath = std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + FileIOKeys::SCENE_ASSETS_FILEPATH;
				WriteFileJson(rootFilePath, d);
				Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved all scene assets to: " << rootFilePath << std::endl;
			}

			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save all scene assets." << std::endl;
			return false;
		}
	}
}
