#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Graphics.h" // Assuming Framework::Texture is defined here

class ResourceManager {
public:
    template<typename ResourceType>
	void add(const std::string& name, std::shared_ptr<ResourceType> resource); // Add resource with name

    template<typename ResourceType>
	std::shared_ptr<ResourceType> get(const std::string& name); // Retrieve resource by name

    template<typename ResourceType>
	void clear(); // Clear resources of a specific type

	void clearAll(); // Clear all resources

private:
    // Declare the template method
    template<typename ResourceType>
    std::unordered_map<std::string, std::shared_ptr<ResourceType>>& getMap();

    // Resource maps
    std::unordered_map<std::string, std::shared_ptr<Framework::Texture>> textureMap;
};
