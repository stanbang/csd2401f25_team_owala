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
#pragma once //Ensure the file is only included once during compilation
#include <xhash>
using Entity = unsigned int; // Define Entity as an alias for unsigned int, all "Entity" usage is essentially unsigned int usage
extern Entity nextEntityID; // Global var to track next available entity ID

/**
 * \brief Returns a new entity ID and increments the global nextEntityID variable.
 * \return Entity - A new unique entity ID.
 */
Entity createEntity();