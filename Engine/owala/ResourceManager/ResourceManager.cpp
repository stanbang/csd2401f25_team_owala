#include "ResourceManager.h"
#include "Graphics.h"
#include "FontManager.h"

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
	loadTexture("playerSprite_DuckyWucky", "Assets/duck-rgba-256.tex");
	loadTexture("RonaldSprite_Platform", "Assets/Ronald.png");
	loadTexture("Background", "Assets/Env_Level2_PH.png");
	LoadFont("minecraft_font", "Assets/Fonts/minecraft.ttf");
	LoadFont("kremlin_font", "Assets/Fonts/Kremlin.ttf");
}

void ResourceManager::clearAll() {
	resourceMap.clear();
	fontsMap.clear();
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

// Loads a font
bool ResourceManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int size) {
	FT_Library library = FontManager::GetInstance().GetLibrary();
	Logger logger;

	// Check if the library is valid (e.g., if FT_Init_FreeType failed)
	if (!library) {
		logger.Info(DebugType::ResourceManager, "FreeType Library not initialized.");
		return false;
	}

	// Font already loaded
	if (fontsMap.count(name) > 0) {
		logger.Info(DebugType::ResourceManager, "Already loaded font': " + fontPath + "'");
		return true;
	}
	
	// Load new font & store in resource manager's font map
	auto newFont = std::make_unique<Font>();
	if (newFont->Load(library, fontPath, size)) {
		fontsMap[name] = std::move(newFont);
		logger.Info(DebugType::ResourceManager, "Loaded font '" + fontPath + "' with size " + std::to_string(size));
		return true;
	}

	// Failed to load
	logger.Info(DebugType::ResourceManager, "Failed to load font from '" + fontPath + "'");
	return false;
}

// Retrieves a font from the cache
Font* ResourceManager::GetFont(const std::string& name) {
	Logger logger;
	if (fontsMap.count(name) > 0) {
		return fontsMap[name].get();
	}
	logger.Info(DebugType::ResourceManager, "Could not find font '" + name + "'");
	return nullptr;
}

// Unloads a font
void ResourceManager::UnloadFont(const std::string& name) {
	Logger logger;
	logger.Info(DebugType::ResourceManager, "Unloaded font '" + name + "'");
	fontsMap.erase(name);
}

// Specialization for Texture
std::unordered_map<std::string, Value>& ResourceManager::getMap() {
	return resourceMap;
}

//template<>
//std::unordered_map<std::string, std::shared_ptr<Texture>>& ResourceManager::getMap<Texture>() {
    //return textureMap;
//}
