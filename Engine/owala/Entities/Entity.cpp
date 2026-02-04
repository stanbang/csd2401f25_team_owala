/*****************************************************************//**
 * \file	Entity.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author

 *
 * \brief	Contains declaratiosn to create an entity and an entityID variable.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "Entity.h"

Entity nextEntityID = 1; // 0 is reserved for invalid/null entity

Entity createEntity() {
    return nextEntityID++;
}