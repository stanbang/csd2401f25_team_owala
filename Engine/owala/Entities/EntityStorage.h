#pragma once
#include "Entity.h"
#include "Components.h"
#include "SpriteComponents.h"
#include "SceneComponents.h"
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
		//Tags
		std::unordered_map<Entity, PlayerTag>		playerTag;
		std::unordered_map<Entity, PlatformTag>		platformTag;
		std::unordered_map<Entity, BackgroundTag>	backgroundTag;
		std::unordered_map<Entity, EnemyTag>		enemyTag;

		//Basic Components
		std::unordered_map<Entity, PlayerStats> playerStats;
		std::unordered_map<Entity, Position>     position;
		std::unordered_map<Entity, Velocity>     velocity;
		std::unordered_map<Entity, Acceleration> acceleration;
		std::unordered_map<Entity, Triangle>     triangle;
		std::unordered_map<Entity, RigidBody>    rigidBody;    
		std::unordered_map<Entity, AABBCollider> colliders; 
		std::unordered_map<Entity, PlayerState> playerState;
		std::unordered_map<Entity, RotationFlag> rotationFlag;
		std::unordered_map<Entity, TrapPlatformComponent> trapPlatform;
		std::unordered_map<Entity, ScalingPlatformComponent> scalingPlatform;

		std::unordered_map<Entity, TextureComponent>      tex; //Texture Component
		std::unordered_map<Entity, AudioComponent>        audio; //Audio Component
		std::unordered_map<Entity, AnimatedTextureComponent> animatedTex; //Animated Texture Component
		std::unordered_map<Entity, TextComponent>	text; // Text Component

		// Scene Components
		std::unordered_map<Entity, SceneOwned>		sceneOwned; // Tag to indicate which scene this entity belongs to
		std::unordered_map<Entity, PersistentTag>	persistentTag; // Tag to indicate if the entity should persist across scene changes
		std::unordered_map<Entity, ActiveTag>		activeTag; // Tag to indicate if entity is to be loaded in the scene

		//Sprite Components
		//std::unordered_map<Entity, Sprite>       sprite;    // single sprite
		//std::unordered_map<Entity, Sprites>      sprites;   // multiple frames (animations)
	};
}