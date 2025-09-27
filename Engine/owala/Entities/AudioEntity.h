/*****************************************************************//**
 * \file   AudioEntity.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *      
 *
 * \brief  Defines function to create background audio entity.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>
#include "Entity.h"
#include "Components.h"
#include "EntityStorage.h"
#include "EntityManager.h"

namespace Framework{
	Entity createBGAudio(EntityStorage& entityStorage,
		PlayerStats stats,
		Vector2D halfSize,
		std::string audioID);
}