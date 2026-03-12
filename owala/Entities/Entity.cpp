/*********************************************************************//**
 * \file   Entity.cpp
 *
 * \primary_author
 *      Ng Jing Wen (100%)
 *
 * \brief   Implementation of entity creation and ID management for the ECS.
 *
 * \details
 *      This file contains the global entity counter and the implementation
 *      of createEntity(). The counter starts at 1 to reserve entity 0 as
 *      an invalid/null entity. Each call to createEntity() increments the
 *      counter and returns a new unique ID.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the
 *      prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "Entity.h"

 /**
  * @brief Global counter for generating unique entity IDs.
  *
  * Initialized to 1 because entity 0 is reserved as invalid/null.
  * This ensures all created entities have valid IDs > 0.
  */
Entity nextEntityID = 1; // 0 is reserved for invalid/null entity

/**
 * @brief Creates a new unique entity ID.
 *
 * Increments the global nextEntityID counter and returns the new value.
 * This function is thread-safe only if called from the main thread
 * (as is typical in game engines).
 *
 * @return Entity A new unique entity identifier (> 0)
 */
Entity createEntity() {
    return nextEntityID++;
}