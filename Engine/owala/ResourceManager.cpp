#include "ResourceManager.h"
#include "Graphics.h"

using namespace Framework;

void ResourceManager::clearAll() {
    textureMap.clear();
}

bool ResourceManager::loadTexture(const std::string& name, const std::string& filepath) {
	Texture tex = gfx_->loadTextureFromFile(filepath.c_str());
	if (tex == 0) {
		std::cout << "Failed to load texture from '" << filepath << "'" << std::endl;
		return false;
	}

	std::cout << "Loaded texture '" << name << "' from '" << filepath << "'" << std::endl;

	auto texturePtr = std::make_shared<Texture>(tex); // Wrap in shared_ptr
	add<Texture>(name, texturePtr); // Add to resource manager's unordered_map
    return true;
}

// Specialization for Texture
template<>
std::unordered_map<std::string, std::shared_ptr<Texture>>& ResourceManager::getMap<Texture>() {
    return textureMap;
}
