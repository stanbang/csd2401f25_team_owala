/*****************************************************************//**
* \file   Platform.h
*
* \primary_author
*      Ng Jing Wen
* \secondary_author
*
*
* \brief	Contains declarations to create a platform entity.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "Entity.h"
#include "Components.h"
#include "EntityStorage.h"
#include "FileUtility.h"
#include <string>

namespace Framework{
	Entity createPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID);
}