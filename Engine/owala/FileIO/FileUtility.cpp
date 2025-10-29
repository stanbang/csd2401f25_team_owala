#include "FileUtility.h"

namespace Framework {
	const std::string FileIOSystem::WIN_CONFIG_FILEPATH = "Data/config.json";
	const std::string FileIOSystem::PLAYER_CONFIG_FILEPATH = "Data/player.json";
	const std::string FileIOSystem::SCENE_ASSETS_FILEPATH = "../../Data/scenes.json";
	const std::string FileIOSystem::SCENES_FILEPATH_PREFIX = "../../Data/Scenes/";

	std::string FileIOSystem::SanitizeFileName(const std::string& name) {
		std::string sanitized_name = name;
		// 1. Convert to lowercase
		std::transform(sanitized_name.begin(), sanitized_name.end(), sanitized_name.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// 2. Replace spaces with underscores
		std::replace(sanitized_name.begin(), sanitized_name.end(), ' ', '_');

		// 3. Remove other forbidden characters (optional, but recommended)
		// sanitized_name.erase(std::remove_if(sanitized_name.begin(), sanitized_name.end(), 
		//     [](char c) { return !std::isalnum(c) && c != '_'; }), sanitized_name.end());

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

	Vector2D FileIOSystem::LoadWindowConfig() {
		std::string logMsg{};
		rapidjson::Document d;
		
		//Read the file name passed in as a path
		if (!ReadFile(WIN_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read config.json. Defaulting to 1920x1080" << std::endl;
			return Vector2D(1920.0f, 1080.0f);
		}

		// Check if the JSON file contains the respective members expected of the config.json file
		if (!d.HasMember("windowConfig") || !d["windowConfig"].IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing windowConfig in config.json. Defaulting to 1920x1080" << std::endl;
			return Vector2D(1920.0f, 1080.0f);
		}

		// Extract windowConfig object
		const rapidjson::Value& config = d["windowConfig"];

		// Check that extracted data has the expected properties
		if (!config.HasMember("width") || !config["width"].IsInt() ||
			!config.HasMember("height") || !config["height"].IsInt()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing data in config.json. Defaulting to 1920x1080" << std::endl;
			return Vector2D(1920.0f, 1080.0f);
		}

		Vector2D windowSize = { config["width"].GetFloat() ,config["height"].GetFloat() };

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Config data read from config.json: Window Size = " << windowSize.x << " x " << windowSize.y << std::endl;
		return windowSize;
	}

	PlayerStats FileIOSystem::LoadPlayerStats() {
		std::string logMsg{};
		rapidjson::Document d;
		//Read the file name passed in as a path
		if (!ReadFile(PLAYER_CONFIG_FILEPATH, d)) {
			Logger::Get().Error(DebugType::FileIOSystem) <<  "Invalid or missing playerStats in player.json" << std::endl;
		}

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("playerStats") || !d["playerStats"].IsObject()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing playerStats in player.json" << std::endl;
		}

		const rapidjson::Value& stats = d["playerStats"]; // Extract the playerStats data from the JSON file

		// Check if moveSpeed member exists in playerStats
		if (!stats.HasMember("moveSpeed") || !stats.HasMember("positionX") || !stats.HasMember("positionY")) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing playerStats in player.json" << std::endl;
		}

		const rapidjson::Value& playerX = stats["positionX"];
		const rapidjson::Value& playerY = stats["positionY"];

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Player Position read from player.json: X: " << playerX.GetFloat() << " Y: " << playerY.GetFloat() << std::endl;

		Vector2D position{ playerX.GetFloat(), playerY.GetFloat() };

		// Since moveSpeed is a float, convert it to float before storing it in the playerStats object
		PlayerStats playerStats = { stats["moveSpeed"].GetFloat(), Position(position)};

		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Player data read from player.json: moveSpeed: " << playerStats.moveSpeed << std::endl;

		return playerStats;
	}

	AnimationInfo FileIOSystem::LoadAnimationInfo(std::string filepath) {
		rapidjson::Document d;
		ReadFile(filepath, d); //Read the file name passed in as a path

		// Check if the JSON file contains the respective members expected of a playerStats.json file
		if (!d.HasMember("Animation") || !d["Animation"].IsObject()) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing animation data in " + filepath);
		}

		const rapidjson::Value& stats = d["Animation"]; // Extract the animation data from the JSON file

		// Check if animation members exists in Animation
		if (!stats.HasMember("frameCount") || !stats.HasMember("frameDuration") || !stats.HasMember("isLooping") || !stats.HasMember("startFrame")) {
			throw GameException(DebugType::FileIOSystem, "Invalid or missing properties in " + filepath);
		}

		const rapidjson::Value& frameCount = stats["frameCount"];
		const rapidjson::Value& frameDuration = stats["frameDuration"];
		const rapidjson::Value& isLooping = stats["isLooping"];
		const rapidjson::Value& startFrame = stats["startFrame"];


		// Debugging output
		Logger::Get().Info(DebugType::FileIOSystem) << "Animation Data read from " << filepath << ": frameCount: " << frameCount.GetInt() << ", frameDuration : " << frameDuration.GetFloat() << ", isLooping : " << isLooping.GetBool() << ", startFrame : " << startFrame.GetInt() << std::endl;

		//Create AnimationComponent from data
		AnimationInfo anim{ frameCount.GetInt(), frameDuration.GetFloat(), isLooping.GetBool(), startFrame.GetInt()};

		return anim;
	}

	bool FileIOSystem::LoadSceneAssets(rapidjson::Document& doc) {
		rapidjson::Document d;
		if (!ReadFile(SCENE_ASSETS_FILEPATH, d) || !d.IsArray()) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Invalid data in: " << SCENE_ASSETS_FILEPATH << std::endl;
			return false;
		}

		doc.Swap(d);
		return true;
	}

	bool FileIOSystem::SaveSceneAsset(const SceneAsset& sceneAsset) {
		try {
			Logger::Get().Info(DebugType::FileIOSystem) << "Saving scene asset <" << sceneAsset.name << "> to: " << SCENE_ASSETS_FILEPATH << std::endl;
			rapidjson::Document d;
			if (!ReadFile(SCENE_ASSETS_FILEPATH, d) || !d.IsArray()) {
				d.SetArray();
			}
			
			rapidjson::Document::AllocatorType& alloc = d.GetAllocator();

			bool sceneAssetExists = false;
			for (rapidjson::SizeType i = 0; i < d.GetArray().Size(); ++i) {
				rapidjson::Value& val = d.GetArray()[i];
			
				if (!val.IsObject() || !val.HasMember(Serialization::SCENE_ASSET_NAME) || !val.HasMember(Serialization::SCENE_ASSET_PATH) /*|| !val.HasMember("isLoaded")*/) {
					Logger::Get().Error(DebugType::FileIOSystem) << "Invalid asset structure encountered in: " << SCENE_ASSETS_FILEPATH << std::endl;
					return false;
				}

				if (val[Serialization::SCENE_ASSET_NAME].GetString() == sceneAsset.name) {
					sceneAssetExists = true;
					val[Serialization::SCENE_ASSET_PATH].SetString(sceneAsset.path.c_str(), alloc);
					//val["isLoaded"].SetBool(sceneAsset.isLoaded);
					break;
				}
			}

			if (!sceneAssetExists) {
				rapidjson::Value asset(kObjectType);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_NAME), rapidjson::Value(sceneAsset.name.c_str(), alloc).Move(), alloc);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_PATH), rapidjson::Value(sceneAsset.path.c_str(), alloc).Move(), alloc);
				//asset.AddMember("isLoaded", rapidjson::Value(sceneAsset.isLoaded).Move(), alloc);
				d.PushBack(asset, alloc);
			}

			WriteFileJson(SCENE_ASSETS_FILEPATH, d);
			Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved scene asset <" << sceneAsset.name << "> to: " << SCENE_ASSETS_FILEPATH << std::endl;
			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save scene asset <" << sceneAsset.name << "> to: " << SCENE_ASSETS_FILEPATH << std::endl;
			return false;
		}
	}

	bool FileIOSystem::SaveAllSceneAssets(const std::unordered_map<SceneGUID, SceneAsset>& allAssets) {
		try {
			Logger::Get().Info(DebugType::FileIOSystem) << "Saving all " << allAssets.size() << " scene assets to: " << SCENE_ASSETS_FILEPATH << std::endl;

			// 1. Create a new document with an array root
			rapidjson::Document d;
			d.SetArray();
			rapidjson::Document::AllocatorType& alloc = d.GetAllocator();

			// 2. Iterate through the complete C++ collection
			for (const auto& [id, sceneAsset] : allAssets) {
				rapidjson::Value asset(kObjectType);

				// 3. Add all members (Name, Path)
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_NAME), rapidjson::Value(sceneAsset.name.c_str(), alloc).Move(), alloc);
				asset.AddMember(rapidjson::StringRef(Serialization::SCENE_ASSET_PATH), rapidjson::Value(sceneAsset.path.c_str(), alloc).Move(), alloc);
				//asset.AddMember("isLoaded", rapidjson::Value(sceneAsset.isLoaded).Move(), alloc);

				// 4. Push the complete object onto the document's root array
				d.PushBack(asset, alloc);
			}

			// 5. Write the entire document to the file (one single I/O operation)
			WriteFileJson(SCENE_ASSETS_FILEPATH, d);

			Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved all scene assets." << std::endl;
			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::FileIOSystem) << "Failed to save all scene assets." << std::endl;
			return false;
		}
	}
}
