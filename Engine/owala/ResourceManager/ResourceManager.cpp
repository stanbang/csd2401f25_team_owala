#include "ResourceManager.h"
#include "AudioSystem.h"
#include "Graphics.h"

using namespace Framework;

void ResourceManager::Initialize() {
	/* --- HOW TO INITIALIZE RESOURCES --- 
	Assets are stored by the Resource Manager as a shared pointer (shared_ptr) inside an unordered_map.
	This allows multiple systems to share the same resource without duplicating memory.

	When accessing a resource, use get<T> to get a weak pointer (weak_ptr).
	Weak pointer allows you to read the resource, but not modify it.

	To modify the resource, convert the weak pointer to a shared pointer using lock(). (E.g. shared_ptr<T> ptr = weakPtr.lock();)
	Then dereference it to get the actual resource. (E.g. T resource = *ptr;)

	E.g.
		x = 5
		weak_ptr<int> x_weakPtr = get<int>("x") // Get weak_ptr to int resource "x"
		shared_ptr<int> x_sharedPtr = x_weakPtr.lock(); // Convert weak_ptr to shared_ptr
		int x_value = *x_sharedPtr; // Dereference shared_ptr to get actual int value
	*/

	//Store texture with key "playerSprite_DuckyWucky" from file "Assets/duck-rgba-256.tex"
	//Access texture later with: Texture_WeakPtr playerTexture = resourceManager_->get<Texture>("playerSprite_DuckyWucky");
	//loadTexture("playerSprite_DuckyWucky", "Assets/duck-rgba-256.tex");
	//loadTexture("RonaldSprite_Platform", "Assets/Ronald.png");
	//loadTexture("Background", "Assets/Env_Level2_Placeholder_Upsized.png");
}

void ResourceManager::clearAll() {
	textureMap.clear();
	soundMap.clear();
}

bool ResourceManager::loadTexture(const std::string& name, const std::string& filepath) {
	Texture tex = gfx_->loadTextureFromFile(filepath.c_str());
	Logger logger;

	if (tex == 0) {
		logger.Info(DebugType::ResourceManager, "Failed to load texture from '" + filepath + "'");
		return false;
	}

	logger.Info(DebugType::ResourceManager, "Loaded texture '" + filepath + "'");

	add<Texture>(name, tex); // Add to resource manager's unordered_map
    return true;
}


bool ResourceManager::loadSound(const std::string& id,
    const char* path,
    bool loop,
    bool stream) {

	Logger logger;
	FMOD::System* system_ = audiosystem_->getFMODSystem();

    if (!system_) return false;
    if (!path)     return false;

    if (!system_) {
        std::fprintf(stderr, "[Audio] System not initialized\n");
        return false;
    }
    if (!path) {
        std::fprintf(stderr, "[Audio] Null path for id '%s'\n", id.c_str());
        return false;
    }

    // Check if file exists
    FILE* test = fopen(path, "rb");
    if (!test) {
        std::fprintf(stderr, "[Audio] File not found: %s\n", path);
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

	logger.Info(DebugType::ResourceManager, "Loaded sound '" + std::string(path) + "'");
	add<Audio>(id, std::unique_ptr<Audio, FMODDeleter>(snd));
    return true;
}

std::unordered_map<std::string, std::unique_ptr<Texture>>& ResourceManager::getTextureMap() {
    return textureMap;
}

std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>>& ResourceManager::getSoundMap() {
    return soundMap;
}


void ResourceManager::Shutdown() {
    Logger logger;
    logger.Info(DebugType::ResourceManager, "ResourceManager shutdown: freeing all resources.");

    // Clear all maps
	clearAll();
}