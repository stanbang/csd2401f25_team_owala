using Entity = unsigned int; // Define Entity as an alias for unsigned int, all "Entity" usage is essentially unsigned int usage
Entity nextEntityID = 0; // Global var to track next available entity ID

/**
 * @brief Returns a new entity ID and increments the
 * global nextEntityID variable.
 * 
 * \return Entity - A new unique entity ID.
 */
Entity createEntity() {
	return nextEntityID++;
}