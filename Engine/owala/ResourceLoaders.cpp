#pragma once
#include "FileUtility.h"
#include "Graphics.h"

namespace ResourceManager {

	using namespace Framework;

	//Texture Loader
	std::shared_ptr<Texture> TextureLoader(Framework::Graphics& gfx, Framework::FileIOSystem& fileIO, const std::string& path) {
		//Load texture using graphics system
		Texture tex = gfx.loadTextureFromFile(path.c_str(), false);
		return std::make_shared<Texture>(tex);
	};

	//

}
