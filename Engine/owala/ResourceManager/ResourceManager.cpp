#include "ResourceManager.h"
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
	loadTexture("playerSprite_DuckyWucky", "Assets/duck-rgba-256.tex");
}

void ResourceManager::clearAll() {
    resourceMap.clear();
}

bool ResourceManager::loadTexture(const std::string& name, const std::string& filepath) {
	Texture tex = gfx_->loadTextureFromFile(filepath.c_str());
	if (tex == 0) {
		std::cout << "Failed to load texture from '" << filepath << "'" << std::endl;
		return false;
	}

	std::cout << "Loaded texture '" << name << "' from '" << filepath << "'" << std::endl;

	add<Texture>(name, tex); // Add to resource manager's unordered_map
    return true;
}

// Specialization for Texture
std::unordered_map<std::string, Value>& ResourceManager::getMap() {
    return resourceMap;
}
