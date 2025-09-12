#pragma once
#include "Entity.h"
#include "Components.h"
#include <unordered_map> //For a hash map to store entity-component pairs

/*	Hash maps to store key-value pairs of entity IDs 
	and their respective components. 
	
	Can be access with e.g. position[2] which means
	position of entity with ID of 2. 
	
	Since Entity is basically an alias of ints, we can 
	actually pass in the entity itself upon creation to set 
	it's initial values.
	
	E.g.
	Entity player = createEntity()
	positions[player] = Position{0,0} // Start at 0,0 coordinates
	*/
namespace Framework {
	class EntityStorage{
	public:
		std::unordered_map<Entity, PlayerTag>    playerTag;
		std::unordered_map<Entity, PlatformTag>  platformTag;
		std::unordered_map<Entity, EnemyTag>     enemyTag;

		std::unordered_map<Entity, Position>     position;
		std::unordered_map<Entity, Velocity>     velocity;
		std::unordered_map<Entity, Acceleration> acceleration;
		std::unordered_map<Entity, Triangle>     triangle;
		std::unordered_map<Entity, RigidBody>    rigidBody;    
		std::unordered_map<Entity, AABBCollider> colliders;    
	};
}