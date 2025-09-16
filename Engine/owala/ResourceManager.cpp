#include "ResourceManager.h"

// Template method definitions
template<typename ResourceType>
void ResourceManager::add(const std::string& name, std::shared_ptr<ResourceType> resource) {
    auto& map = getMap<ResourceType>();

	if (map.find(name) != map.end()) {
		throw std::runtime_error("Resource with name '" + name + "' already exists.");
	}

    map[name] = resource;
}

template<typename ResourceType>
std::shared_ptr<ResourceType> ResourceManager::get(const std::string& name) {
    auto& map = getMap<ResourceType>();
    auto it = map.find(name);
    return (it != map.end()) ? it->second : nullptr; 
}

template<typename ResourceType>
void ResourceManager::clear() {
    getMap<ResourceType>().clear();
}

void ResourceManager::clearAll() {
    textureMap.clear();
}

// Specialization for Framework::Texture
template<>
std::unordered_map<std::string, std::shared_ptr<Framework::Texture>>& ResourceManager::getMap<Framework::Texture>() {
    return textureMap;
}
