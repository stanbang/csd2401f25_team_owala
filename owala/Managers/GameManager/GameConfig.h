/*****************************************************************//**
 * \file   GameConfig.h
 * 
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 * 
 * \brief  Lightweight header for GameManager::GameConfig so that
 *         low-level systems (e.g. FileIOSystem) can reference the
 *         struct without pulling in the full GameManager header and
 *         creating a circular include.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include <string>
#include <vector>

namespace Framework {
	struct GameConfig {
		std::string startupScene  = "";
		std::string loadingScene  = "";
		std::vector<std::string> overlayScenes; // scenes that pause physics (e.g. pause/options)
		std::vector<std::string> menuScenes;    // non-level scenes for IsLevelScene()
	};
}
