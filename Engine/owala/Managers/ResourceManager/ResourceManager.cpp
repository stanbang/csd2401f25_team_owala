#include "ResourceManager.h"
#include "AudioSystem.h"
#include "Graphics.h"
#include "GL/glew.h"
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
	
}

void ResourceManager::Update(float dt) {
	(void)dt; //Suppress unused variable warning
	// Intentionally left empty
}

// Clear all resources
void ResourceManager::clearAll() {
	textureMap.clear();
	soundMap.clear();
	fontMap.clear();
	animationMap.clear();
}

// Loads a texture from file and stores it inside the resource manager
bool ResourceManager::loadTexture(const std::string& name, const std::string& filepath) {
	Texture tex = gfx_->loadTextureFromFile(filepath.c_str());

	if (tex == 0) {
		Logger::Get().Info(DebugType::ResourceManager) << "Failed to load texture from '" << filepath << "'" << std::endl;
		return false;
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded texture '" << filepath << "'" << std::endl;

	add<Texture>(name, tex); // Add to resource manager's unordered_map
    return true;
}

// Loads a sound from file and stores it inside the resource manager
bool ResourceManager::loadSound(const std::string& id,
    const char* path,
    bool loop,
    bool stream) {

	FMOD::System* system_ = audiosystem_->getFMODSystem();

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
    audiosystem_->check(r, "System::createSound");
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

// Loads a font from file and stores it inside the resource manager
bool ResourceManager::loadFont(const std::string& name, const std::string& fontPath, unsigned int size) {
	FT_Library library = FreeTypeManager::GetInstance().GetLibrary();

	// Check if the library is valid (e.g., if FT_Init_FreeType failed)
	if (!library) {
		Logger::Get().Error(DebugType::ResourceManager) << "FreeType Library not initialized." << std::endl;
		return false;
	}

	// Font already loaded
	if (fontMap.count(name) > 0) {
		Logger::Get().Info(DebugType::ResourceManager) <<  "Already loaded font': " << fontPath << "'" << std::endl;
		return true;
	}
	
	// Load new font & store in resource manager's font map
	auto newFont = std::make_unique<Font>();
	if (newFont->Load(library, fontPath, size)) {
		add<Font>(name, std::move(newFont)); // Add to resource manager's unordered_map
		Logger::Get().Info(DebugType::ResourceManager) << "Loaded font '" << fontPath << "' with size " << size << std::endl;
		return true;
	}

	// Failed to load
	Logger::Get().Error(DebugType::ResourceManager) << "Failed to load font from '" << fontPath << "'" << std::endl;
	return false;
}

// Loads animation info from file and stores it inside the resource manager
bool ResourceManager::loadAnimationInfo(const std::string& name, const std::string& filepath) {
	FileIOSystem fileIO;
	AnimationInfo animState = fileIO.LoadAnimationInfo(filepath);

	if (animState.frameCount == 0) {
		Logger::Get().Error(DebugType::ResourceManager) << "Failed to load animation info from '" << filepath << "'" << std::endl;
		return false;
	}

	Logger::Get().Info(DebugType::ResourceManager) << "Loaded animation info '" << filepath << "'" << std::endl;
	add<AnimationInfo>(name, std::move(std::make_unique<AnimationInfo>(animState))); // Add to resource manager's unordered_map
	return true;
}

// Shutdown and free all resources
void ResourceManager::Shutdown() {
	Logger::Get().Info(DebugType::ResourceManager) << "ResourceManager shutdown: freeing all resources." << std::endl;

    // Clear all maps
	clearAll();
}