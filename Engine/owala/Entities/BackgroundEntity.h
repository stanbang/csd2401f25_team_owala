/*****************************************************************//**
 * \file   BackgroundEntity.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "Entity.h"
#include "EntityStorage.h"
#include <string>

namespace Framework{
	Entity createBackground(EntityStorage& entityStorage, Vector2D pos, std::string textureID);
}	