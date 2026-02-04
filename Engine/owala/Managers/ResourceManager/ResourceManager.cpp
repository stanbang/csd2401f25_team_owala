/*****************************************************************//**
* \file   ResourceManager.cpp
*
* \primary_author
*      Ng Jing Wen
* \secondary_author
*
* \brief    Contains declarations and definitions of a ResourceManager utilising templates
*           that can load and store various resource types like textures, audio and fonts.
*           It maintains ownership of resources using smart pointers with custom deleters.
*           Ensuring that resources are properly released when no longer needed or out of scope.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "ResourceManager.h"
#include "GameManager.h"
#include "FileIO/FileUtility.h"

using namespace Framework;

void Framework::TextureDeleter::operator()(Texture* texPtr) const noexcept {
    if (texPtr && *texPtr != 0) {
        // *texPtr dereferences the pointer to get the actual Texture (GLuint) handle
        glDeleteTextures(1, texPtr);
        *texPtr = 0; // Set ID to 0 after deletion
    }
    // The memory pointed to by texPtr (the Texture object) must be deleted here
    delete texPtr;
}

void Framework::FMODDeleter::operator()(FMOD::Sound* s) const noexcept {
    if (s) {
        s->release(); // Release without logging
    }
}

void ResourceManager::Initialize() {
	// Scan asset directories recursively to auto-map filenames to paths
	try {
		ScanAssetDirectory("Assets/Sprites");
		ScanAssetDirectory("Assets/Audio");
		ScanAssetDirectory("Assets/Fonts");
		ScanAssetDirectory("Data/AnimationData");
		ScanAssetDirectory("Data/Prefabs/Props");
		ScanAssetDirectory("Data/Prefabs/Tiles");
		ScanAssetDirectory("Data/Prefabs/TrapPlatforms");
		LoadTextureAnimationMappings("Data/textureToAnimationMapping.json");
		LoadAnimationProfiles("Data/stateToAnimationMapping.json");
	}
	catch (const fs::filesystem_error& e) {
		Logger::Get().Error(DebugType::ResourceManager) << "Failed to scan assets: " << e.what();
	}

	// Start worker thread for asynchronous loading if needed
	m_isLoading = true;
	m_workerThread = std::thread(&ResourceManager::WorkerThreadLoop, this);
}

void ResourceManager::Update() {
	// Systems used by ResourceManager
	Graphics& gfx_ = *GameManager::GetInstance().GetSystems().gfx_;

	// Upload a limited number of textures per frame to maintain high FPS
	int uploadsThisFrame = 0;
	const int MAX_UPLOADS_PER_FRAME = 2;

	std::lock_guard<std::mutex> lock(m_queueMutex);

	while (!m_dataReadyToUpload.empty() && uploadsThisFrame < MAX_UPLOADS_PER_FRAME) {
		AsyncTextureData item = m_dataReadyToUpload.front();
		m_dataReadyToUpload.pop();

		// GPU Upload (Fast, Main Thread)
		Texture tex = gfx_.CreateTextureFromRawData(item.rawData);

		// Add to the map
		if (tex != 0) {
			std::stringstream ss;
			ss << "[Main Thread   " << std::this_thread::get_id() << "] Uploaded to GPU: " << item.name;
			Logger::Get().Info(DebugType::ResourceManager) << ss.str() << "\n";
			add<Texture>(item.name, tex);
		}

		// Free CPU memory
		gfx_.FreeImageRawData(item.rawData);

		m_itemsProcessed++;
		uploadsThisFrame++;
	}
}

// Clear all resources
void ResourceManager::clearAll() {
	textureMap.clear();
	soundMap.clear();
	fontMap.clear();
	animationMap.clear();
	prefabMap.clear();
}

// Scans a directory and maps asset filenames to their full paths
void ResourceManager::ScanAssetDirectory(const fs::path& directoryPath) {
	if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
		Logger::Get().Info(DebugType::ResourceManager) << "Asset directory not found: " << directoryPath.string();
		return;
	}

	for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
		if (entry.is_regular_file()) {
			std::string filename = entry.path().filename().string();

			// Check for duplicates. You can decide to warn or just overwrite.
			if (assetPathMap_.count(filename)) {
				Logger::Get().Info(DebugType::ResourceManager)
					<< "Duplicate asset key found: " << filename
					<< ". Overwriting with new path: " << entry.path().string();
			}

			assetPathMap_[filename] = entry.path();
		}
	}
}

// Loads a texture by name using the scanned asset paths
bool ResourceManager::loadTexture(const std::string& name) {
	// Check if we scanned this asset
	auto it = assetPathMap_.find(name);
	if (it == assetPathMap_.end()) {
		Logger::Get().Error(DebugType::ResourceManager)
			<< "Cannot auto-load texture: Key '" << name << "' not found. Was the directory scanned?\n";
		return false;
	}

	// We found it, now call the original loadTexture with the full path
	const std::string& fullPath = it->second.string();
	return loadTexture(name, fullPath); // Calls your existing function
}

// OVERLOAD Loads a texture from file and stores it inside the resource manager
bool ResourceManager::loadTexture(const std::string& name, const std::string& filepath) {
	Texture tex = GameManager::GetInstance().GetSystems().gfx_->loadTextureFromFile(filepath.c_str());

	if (tex == 0) {
		Logger::Get().Info(DebugType::ResourceManager) << "Failed to load texture from '" << filepath << "'" << std::endl;
		return false;
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded texture '" << filepath << "'" << std::endl;

	add<Texture>(name, tex); // Add to resource manager's unordered_map
    return true;
}

bool ResourceManager::loadSound(const std::string& name, bool loop) {
	auto it = assetPathMap_.find(name);
	if (it == assetPathMap_.end()) {
		Logger::Get().Error(DebugType::ResourceManager)
			<< "Cannot auto-load sound: Key '" << name << "' not found.\n";
		return false;
	}

	return loadSound(name, it->second.string().c_str(), loop, false);
}

// Loads a sound from file and stores it inside the resource manager
bool ResourceManager::loadSound(const std::string& id,
    const char* path,
    bool loop,
    bool stream) {

	AudioSystem& audiosystem_ = *GameManager::GetInstance().GetSystems().audio_;

	FMOD::System* system_ = audiosystem_.getFMODSystem();

    if (!system_) return false;
    if (!path)     return false;

    if (!system_) {
		Logger::Get().Error(DebugType::ResourceManager) << "Audio System not initialized" << std::endl;
        return false;
    }
    if (!path) {
		Logger::Get().Error(DebugType::ResourceManager) << "Audio: Null path for id: " << id << std::endl;
        return false;
    }

    // Check if file exists
    FILE* test = fopen(path, "rb");
    if (!test) {
		Logger::Get().Error(DebugType::ResourceManager) << "Audio file not found: " << path << std::endl;
        return false;
    }
    fclose(test);

    FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D;
    mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    mode |= stream ? FMOD_CREATESTREAM : 0;

	Audio* snd = nullptr;
    FMOD_RESULT r = system_->createSound(path, mode, nullptr, &snd);
    audiosystem_.check(r, "System::createSound");
    if (r != FMOD_OK || !snd) return false;

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded sound '" <<  path << "'" << std::endl;
	add<Audio>(id, std::unique_ptr<Audio, FMODDeleter>(snd));
    return true;
}

// Getter for the texture map
std::unordered_map<std::string, std::unique_ptr<Texture, TextureDeleter>>& ResourceManager::getTextureMap() {
    return textureMap;
}

// Getter for the sound map
std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>>& ResourceManager::getSoundMap() {
    return soundMap;
}

// Getter for the font map
std::unordered_map<std::string, std::unique_ptr<Font>>& ResourceManager::getFontMap() {
	return fontMap;
}

// Getter for the animation map
std::unordered_map<std::string, std::unique_ptr<AnimationInfo>>& ResourceManager::getAnimationMap() {
	return animationMap;
}

// Getter for the prefab map
std::unordered_map<std::string, std::unique_ptr<EntityPrefab>>& ResourceManager::getPrefabMap() {
	return prefabMap;
}

// Loads a font by name using the scanned asset paths
bool ResourceManager::loadFont(const std::string& name) {
	auto it = assetPathMap_.find(name);
	if (it == assetPathMap_.end()) {
		Logger::Get().Error(DebugType::ResourceManager)
			<< "Cannot auto-load font: Key '" << name << "' not found.\n";
		return false;
	}

	// Calls your loadFont, which uses the default size (48)
	return loadFont(name, it->second.string());
}

// Loads a font from file and stores it inside the resource manager
bool ResourceManager::loadFont(const std::string& fontKey, const std::string& fontPath, unsigned int size) {
	FT_Library library = FreeTypeManager::GetInstance().GetLibrary();

	// Check if the library is valid (e.g., if FT_Init_FreeType failed)
	if (!library) {
		Logger::Get().Error(DebugType::ResourceManager) << "FreeType Library not initialized." << std::endl;
		return false;
	}

	// Font already loaded
	if (fontMap.count(fontKey) > 0) {
		Logger::Get().Info(DebugType::ResourceManager) <<  "Already loaded font': " << fontPath << "'" << std::endl;
		return true;
	}
	
	// Load new font & store in resource manager's font map
	auto newFont = std::make_unique<Font>();
	if (newFont->Load(library, fontPath, size)) {
		add<Font>(fontKey, std::move(newFont)); // Add to resource manager's unordered_map
		Logger::Get().Info(DebugType::ResourceManager) << "Loaded font '" << fontPath << "' with size " << size << std::endl;
		return true;
	}

	// Failed to load
	Logger::Get().Error(DebugType::ResourceManager) << "Failed to load font from '" << fontPath << "'" << std::endl;
	return false;
}

// Loads an animation info by name using the scanned asset paths
bool ResourceManager::loadAnimationInfo(const std::string& name) {
	auto it = assetPathMap_.find(name);
	if (it == assetPathMap_.end()) {
		Logger::Get().Error(DebugType::ResourceManager)
			<< "Cannot auto-load animation: Key '" << name << "' not found.\n";
		return false;
	}

	// Calls original loadAnimationInfo
	return loadAnimationInfo(name, it->second.string());
}

// Loads animation info from file and stores it inside the resource manager
bool ResourceManager::loadAnimationInfo(const std::string& name, const std::string& filepath) {
	AnimationInfo animState = FileIOSystem::LoadAnimationInfo(filepath);

	if (animState.frameCount == 0) {
		Logger::Get().Error(DebugType::ResourceManager) << "Failed to load animation info from '" << filepath << "'" << std::endl;
		return false;
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded animation info '" << filepath << "'" << std::endl;
	add<AnimationInfo>(name, std::move(std::make_unique<AnimationInfo>(animState))); // Add to resource manager's unordered_map
	return true;
}

// Loads a prefab by name using the scanned asset paths
bool ResourceManager::loadPrefab(const std::string& name) {
	auto it = assetPathMap_.find(name);
	if (it == assetPathMap_.end()) {
		Logger::Get().Error(DebugType::ResourceManager) << "Cannot auto-load prefab: Key '" << name << "' not found.\n";
		return false;
	}
	return loadPrefab(name, it->second.string());
}

// Loads a prefab from file and stores it inside the resource manager
bool ResourceManager::loadPrefab(const std::string& name, const std::string& filepath) {
	rapidjson::Document doc;
	if (!FileIOSystem::ReadFile(filepath, doc) || !doc.IsObject()) {
		Logger::Get().Error(DebugType::ResourceManager) << "Failed to open prefab file: " << filepath << std::endl;
		return false;
	}

	auto newPrefab = std::make_unique<EntityPrefab>();
	newPrefab->sourceFile = filepath;

	if (doc.HasMember("name") && doc["name"].IsString()) {
		newPrefab->name = doc["name"].GetString();
	}

	// Load Components
	if (doc.HasMember(Serialization::COMPONENTS) && doc[Serialization::COMPONENTS].IsObject()) {
		const rapidjson::Value& compsObj = doc[Serialization::COMPONENTS];
		for (auto it = compsObj.MemberBegin(); it != compsObj.MemberEnd(); ++it) {
			ComponentData compData;
			compData.type = it->name.GetString();
			compData.data.CopyFrom(it->value, compData.data.GetAllocator());
			newPrefab->components.push_back(std::move(compData));
		}
	}

	// Load Tags
	if (doc.HasMember(Serialization::TAGS) && doc[Serialization::TAGS].IsArray()) {
		const rapidjson::Value& tagsArray = doc[Serialization::TAGS];
		for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
			if (tagsArray[j].IsString()) newPrefab->tags.push_back(tagsArray[j].GetString());
		}
	}

	// Load Layer Info
	if (doc.HasMember(Serialization::ENTITY_LAYER) && doc[Serialization::ENTITY_LAYER].IsInt()) {
		int layerInt = doc[Serialization::ENTITY_LAYER].GetInt();
		CollisionLayer layer = static_cast<CollisionLayer>(layerInt);
		if (layer == CollisionLayer::Platform) newPrefab->layer = "Platform";
		else if (layer == CollisionLayer::Player) newPrefab->layer = "Player";
		else newPrefab->layer = "Default";
	}

	if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER) && doc[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
		newPrefab->renderLayer = doc[Serialization::ENTITY_RENDER_LAYER].GetInt();
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded prefab '" << filepath << "'" << std::endl;
	add<EntityPrefab>(name, std::move(newPrefab));
	return true;
}

// Shutdown and free all resources
void ResourceManager::Shutdown() {
	Logger::Get().Info(DebugType::ResourceManager) << "ResourceManager shutdown: freeing all resources." << std::endl;

	// Stop thread
	m_isLoading = false;
	if (m_workerThread.joinable()) {
		m_workerThread.join();
	}

	// Clear upload queue and free any pending raw data
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		// We need access to the Graphics system to free the raw data.
		// ResourceManager MUST shut down b4 Graphics/GameManager, 
		// otherwise GetSystems() might be invalid or null.
		Graphics* gfx = nullptr;
		if (GameManager::Exists()) {
			gfx = GameManager::GetInstance().GetSystems().gfx_;
		}

		while (!m_dataReadyToUpload.empty()) {
			AsyncTextureData item = m_dataReadyToUpload.front();
			m_dataReadyToUpload.pop();

			if (gfx) {
				// Free the memory
				gfx->FreeImageRawData(item.rawData);

				std::stringstream ss;
				ss << "Freed pending async texture data during shutdown: " << item.name;
				Logger::Get().Info(DebugType::ResourceManager) << ss.str() << std::endl;
			}
			else {
				Logger::Get().Error(DebugType::ResourceManager) << "Leaking texture data " << item.name << " because Graphics system is already gone.";
			}
		}
	}

    // Clear all maps
	clearAll();
}

bool ResourceManager::LoadTextureAnimationMappings(const std::string& mappingFilePath) {
	try {
		rapidjson::Document doc;
		if (!FileIOSystem::ReadFile(mappingFilePath, doc)) {
			Logger::Get().Error(DebugType::ResourceManager)
				<< "Cannot open texture-animation mapping file: " << mappingFilePath;
			return false;
		}

		if (!doc.IsObject() || !doc.HasMember("animations") || !doc["animations"].IsArray()) {
			Logger::Get().Error(DebugType::ResourceManager)
				<< "Invalid mapping file structure. Expected 'animations' array.";
			return false;
		}

		const rapidjson::Value& animations = doc["animations"];
		int loadedCount = 0;

		for (rapidjson::SizeType i = 0; i < animations.Size(); ++i) {
			const rapidjson::Value& entry = animations[i];

			// Validate 'animation_path'
			if (!entry.HasMember("animation_path") || !entry["animation_path"].IsString()) {
				Logger::Get().Info(DebugType::ResourceManager) << "Skipping entry " << i << ": Missing 'animation_path'";
				continue;
			}
			std::string animationPath = entry["animation_path"].GetString();

			// Load the animation info once to validate
			AnimationInfo cachedInfo = FileIOSystem::LoadAnimationInfo(animationPath);
			if (cachedInfo.frameCount == 0) {
				Logger::Get().Error(DebugType::ResourceManager) << "Failed to load animation info from '" << animationPath << "'";
				continue;
			}

			// Check for 'texture_keys' (Array)
			if (entry.HasMember("texture_keys") && entry["texture_keys"].IsArray()) {
				const rapidjson::Value& keys = entry["texture_keys"];

				// Loop through every texture key in the list
				for (rapidjson::SizeType j = 0; j < keys.Size(); ++j) {
					if (keys[j].IsString()) {
						std::string textureKey = keys[j].GetString();

						// Load the SAME animation file for THIS texture key
						add<AnimationInfo>(textureKey, std::make_unique<AnimationInfo>(cachedInfo));
						loadedCount++;
					}
				}
			}
			// Backward Compatibility for old "texture_key" (String)
			else if (entry.HasMember("texture_key") && entry["texture_key"].IsString()) {
				std::string textureKey = entry["texture_key"].GetString();
				add<AnimationInfo>(textureKey, std::make_unique<AnimationInfo>(cachedInfo));
				loadedCount++;
			}
		}

		Logger::Get().Info(DebugType::ResourceManager)
			<< "Mapped " << loadedCount << " textures to animations from " << mappingFilePath;

		return loadedCount > 0;
	}
	catch (const std::exception& e) {
		Logger::Get().Error(DebugType::ResourceManager)
			<< "Exception loading mappings: " << e.what();
		return false;
	}
}

// Adds a texture file to the async loading queue eg. when loading big assets
void ResourceManager::LoadTextureAsync(const std::string& filepath) {
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_filesToLoad.push(filepath);

	m_totalItemsToLoad++;
}

// Background worker thread loop for loading resources asynchronously
void ResourceManager::WorkerThreadLoop() {
	Graphics& gfx_ = *GameManager::GetInstance().GetSystems().gfx_;

	while (m_isLoading) {
		std::string filePath;
		bool hasWork = false;

		// Check for work
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			if (!m_filesToLoad.empty()) {
				filePath = m_filesToLoad.front();
				m_filesToLoad.pop();
				hasWork = true;
			}
		}

		if (hasWork) {
			// Heavy lifting (Disk I/O + Decode)
			std::stringstream ss;
			ss << "[Worker Thread " << std::this_thread::get_id() << "] Loading from Disk: " << filePath;
			Logger::Get().Info(DebugType::ResourceManager) << ss.str() << "\n";

			std::string name = fs::path(filePath).filename().string();

			Graphics::ImageRawData raw = gfx_.LoadImageToCPU(filePath.c_str());

			// Push result to Upload Queue
			{
				std::lock_guard<std::mutex> lock(m_queueMutex);
				m_dataReadyToUpload.push({ name, raw });
			}
		}
		else {
			// Sleep to save CPU if no work
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

float ResourceManager::GetLoadingProgress() const {
	if (m_totalItemsToLoad == 0) return 1.0f;
	return (float)m_itemsProcessed / (float)m_totalItemsToLoad;
}

bool ResourceManager::IsLoadingComplete() const {
	return m_itemsProcessed >= m_totalItemsToLoad;
}

std::string ResourceManager::GetAssetPath(const std::string& key) const {
	auto it = assetPathMap_.find(key);
	if (it != assetPathMap_.end()) {
		return it->second.string();
	}
	return ""; // Not found
}

// Resolves a state key to its corresponding integer ID based on the profile name
int ResolveStateID(const std::string& profileName, const std::string& stateKey) {

	// BIRD ENEMY MAPPING (Matches BirdState Enum)
	if (profileName == "BirdEnemy") {
		if (stateKey == "IDLE")    return 0; // BirdState::IDLE
		if (stateKey == "WAKING")  return 1; // BirdState::WAKING
		if (stateKey == "CHASE")   return 2; // BirdState::CHASE
		if (stateKey == "HOVER")   return 3; // BirdState::HOVER
		if (stateKey == "STUNNED") return 4; // BirdState::STUNNED
		if (stateKey == "ATTACK")  return 5; // BirdState::ATTACK
	}

	// SKELETON ENEMY MAPPING (Matches SkeletonState Enum)
	else if (profileName == "SkeletonEnemy") {
		if (stateKey == "IDLE")    return 0; // SkeletonState::IDLE
		if (stateKey == "CHASE")   return 1; // SkeletonState::CHASE
	}

	// DEFAULT / PLAYER MAPPING (Matches AnimationState Enum)
	// Applies to "Player", "FakePlayer", and anyone else
	else {
		if (stateKey == "IDLE")    return 0; // AnimationState::IDLE
		if (stateKey == "RUNNING") return 1; // AnimationState::RUNNING
		if (stateKey == "JUMPING") return 2; // AnimationState::JUMPING
		if (stateKey == "FALLING") return 3; // AnimationState::FALLING
		if (stateKey == "DEATH")   return 4; // AnimationState::DEATH
		if (stateKey == "HURT")   return 5; // AnimationState::HURT
	}

	return 0; // Default safe fallback
}

// Undefine GetObject macro if defined (to avoid conflicts with RapidJSON)
#ifdef GetObject
#undef GetObject
#endif

bool ResourceManager::LoadAnimationProfiles(const std::string& filepath) {
	rapidjson::Document doc;
	if (!FileIOSystem::ReadFile(filepath, doc)) {
		Logger::Get().Error(DebugType::ResourceManager) << "Failed to load animation profiles: " << filepath;
		return false;
	}

	if (!doc.IsObject()) return false;

	int profilesLoaded = 0;

	// Iterate over profiles: "Player", "BirdEnemy", "FakePlayer"
	const auto& docObj = doc.GetObject();
	for (auto it = docObj.MemberBegin(); it != docObj.MemberEnd(); ++it) {
		std::string profileName = it->name.GetString();

		if (!it->value.IsObject()) continue;

		// Iterate over states: "IDLE", "RUNNING", "CHASE"
		const auto& stateObj = it->value.GetObject();
		for (auto stateIt = stateObj.MemberBegin(); stateIt != stateObj.MemberEnd(); ++stateIt) {
			std::string stateKey = stateIt->name.GetString();   // e.g. "CHASE"
			std::string textureKey = stateIt->value.GetString(); // e.g. "Bird_Chase.png"

			// Directly map the string state to the texture key
			animationProfiles_[profileName][stateKey] = textureKey;
		}
		profilesLoaded++;
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded " << profilesLoaded << " animation profiles.";
	return true;
}


Audio& ResourceManager::getLoopAudio(std::string key) {
	std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>>& map = getSoundMap();
	auto it = map.find(key);
	if (it == map.end()) {
		// Load automatically if missing
		if (loadSound(key, true)) {
			it = map.find(key);
			if (it != map.end()) return *it->second;
		}

		Logger::Get().Error(DebugType::ResourceManager) << "Missing Audio: \"" << key << "\"\n";
		throw std::runtime_error("Missing Audio: " + key);
	}
	return *it->second;
}