/*****************************************************************//**
 * \file   Entity.hpp
 * \brief  Basic entity management for ECS architecture. Creates unique entity IDs every time createEntity() is called.
 * 
 * \author Ng Jing Wen
 * \date   9 September 2025
 *********************************************************************/
#pragma once //Ensure the file is only included once during compilation
using Entity = unsigned int; // Define Entity as an alias for unsigned int, all "Entity" usage is essentially unsigned int usage
Entity nextEntityID = 0; // Global var to track next available entity ID

/**
 * \brief Returns a new entity ID and increments the global nextEntityID variable.
 * \return Entity - A new unique entity ID.
 */
Entity createEntity() {
	return nextEntityID++;
}