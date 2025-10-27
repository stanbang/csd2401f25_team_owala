/*****************************************************************//**
 * \file   EntityStorage.h
 *
 * \primary_author
 *      Jason Lou(60%)
 * \secondary_author
 *		Ng Jing Wen(40%)
 *
 * \brief	Contains the definition of EntityStorage class which stores
 *			unordered maps of entity-component pairs for all components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "Entity.h"
#include "Components.h"
#include "SceneComponents.h"
#include "CameraSystem.h"  
#include <bitset> //For a bitset to track which components an entity has
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
	// Forward Declaration
	class EntityManager;
	class WIPEntityManager;

	enum ComponentType {
		PlayerStatsType = 0,
		PositionType,
		VelocityType,
		AccelerationType,
		TriangleType,
		RigidBodyType,
		AABBColliderType,
		PlayerStateType,
		RotationFlagType,
		TrapPlatformComponentType,
		ScalingPlatformComponentType,
		TextureComponentType,
		AudioComponentType,
		AnimationComponentType,
		TextComponentType,
		CameraType,
		PlayerTagType,
		FakePlayerTagType,
		PlatformTagType,
		BackgroundTagType,
		EnemyTagType,
		SceneOwnedType,
		PersistentTagType,
		ActiveTagType,
		MAX_COMPONENTS
	};

#pragma region COMPONENT TYPE DEFINITIONS
	// --------------------------------------------------------------------------------
	//							/!\	/!\	READ THIS /!\ /!\
	//	         Whenever you create a new component or tag. Please add it in
	//             enum and to the list of ComponentTypeTraits below - Jason
	// --------------------------------------------------------------------------------

	// Generic template
	template <typename T>
	struct ComponentTypeTrait;

	// Specializations for each real component
	template <> struct ComponentTypeTrait<PlayerStats> { static constexpr ComponentType value = PlayerStatsType; };
	template <> struct ComponentTypeTrait<Position> { static constexpr ComponentType value = PositionType; };
	template <> struct ComponentTypeTrait<Velocity> { static constexpr ComponentType value = VelocityType; };
	template <> struct ComponentTypeTrait<Acceleration> { static constexpr ComponentType value = AccelerationType; };
	template <> struct ComponentTypeTrait<Triangle> { static constexpr ComponentType value = TriangleType; };
	template <> struct ComponentTypeTrait<RigidBody> { static constexpr ComponentType value = RigidBodyType; };
	template <> struct ComponentTypeTrait<AABBCollider> { static constexpr ComponentType value = AABBColliderType; };
	template <> struct ComponentTypeTrait<PlayerState> { static constexpr ComponentType value = PlayerStateType; };
	template <> struct ComponentTypeTrait<RotationFlag> { static constexpr ComponentType value = RotationFlagType; };
	template <> struct ComponentTypeTrait<TrapPlatformComponent> { static constexpr ComponentType value = TrapPlatformComponentType; };
	template <> struct ComponentTypeTrait<ScalingPlatformComponent> { static constexpr ComponentType value = ScalingPlatformComponentType; };
	template <> struct ComponentTypeTrait<TextureComponent> { static constexpr ComponentType value = TextureComponentType; };
	template <> struct ComponentTypeTrait<AudioComponent> { static constexpr ComponentType value = AudioComponentType; };
	template <> struct ComponentTypeTrait<AnimationComponent> { static constexpr ComponentType value = AnimationComponentType; };
	template <> struct ComponentTypeTrait<TextComponent> { static constexpr ComponentType value = TextComponentType; };
	template <> struct ComponentTypeTrait<Camera> { static constexpr ComponentType value = CameraType;};
	template <> struct ComponentTypeTrait<PlayerTag> { static constexpr ComponentType value = PlayerTagType; };
	template <> struct ComponentTypeTrait<FakePlayerTag> { static constexpr ComponentType value = FakePlayerTagType; };
	template <> struct ComponentTypeTrait<PlatformTag> { static constexpr ComponentType value = PlatformTagType; };
	template <> struct ComponentTypeTrait<BackgroundTag> { static constexpr ComponentType value = BackgroundTagType; };
	template <> struct ComponentTypeTrait<EnemyTag> { static constexpr ComponentType value = EnemyTagType; };
	template <> struct ComponentTypeTrait<SceneOwned> { static constexpr ComponentType value = SceneOwnedType; };
	template <> struct ComponentTypeTrait<PersistentTag> { static constexpr ComponentType value = PersistentTagType; };
	template <> struct ComponentTypeTrait<ActiveTag> { static constexpr ComponentType value = ActiveTagType; };

#pragma endregion

	using ComponentSignature = std::bitset<MAX_COMPONENTS>; // Bitset to track which components an entity has

	class EntityStorage{
	private:
		std::unordered_map<Entity, ComponentSignature> entitySignatures; // Track components of each entity
		friend class EntityManager; // Allow EntityManager to access private members
	private:
		//Tags
		//std::unordered_map<Entity, PlayerTag>		playerTag;
		//std::unordered_map<Entity, PlatformTag>		platformTag;
		//std::unordered_map<Entity, BackgroundTag>	backgroundTag;
		//std::unordered_map<Entity, EnemyTag>		enemyTag;

		//Basic Components
		std::unordered_map<Entity, EntityProperties> properties;
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
		std::unordered_map<Entity, AnimationComponent> animatedTex; //Animated Texture Component
		std::unordered_map<Entity, TextComponent>	text; // Text Component

		std::unordered_map<Entity, Camera> cameras;//Camera

		// Scene Components
		std::unordered_map<Entity, SceneOwned>		sceneOwned; // Tag to indicate which scene this entity belongs to
		//std::unordered_map<Entity, PersistentTag>	persistentTag; // Tag to indicate if the entity should persist across scene changes
		//std::unordered_map<Entity, ActiveTag>		activeTag; // Tag to indicate if entity is to be loaded in the scene

	//	//Sprite Components
	//	//std::unordered_map<Entity, Sprite>       sprite;    // single sprite
	//	//std::unordered_map<Entity, Sprites>      sprites;   // multiple frames (animations)
	};

	// BELOW IS STILL WIP. DO NOT USE.

	class WIPEntityStorage {
	private:
		// Archetypes is essentially a Set of components
		struct Archetype {
			ComponentSignature signature; // Components in this Archetype
			std::vector<Entity> entities; // Entities that belong to this Archetype

			std::vector<Position> positions;
			std::vector<Velocity> velocities;
			std::vector<Acceleration> accelerations;
			std::vector<RigidBody> rigidBodies;
			std::vector<AABBCollider> aabbColliders;
			std::vector<Triangle> triangles;
			std::vector<TextureComponent> textures;
			std::vector<PlayerStats> playerStats;
			std::vector<PlayerState> playerStates;
			std::vector<RotationFlag> rotationFlags;
			std::vector<SceneOwned> sceneOwned;
		};

		std::vector<Archetype> archetypes; // List of all archetypes
		std::unordered_map<Entity, size_t> entityToArchetype; // Map entity to its archetype index
		std::unordered_map<Entity, ComponentSignature> entitySignatures; // Track components of each entity

		friend class WIPEntityManager; // Allow EntityManager to access private members
	};
}