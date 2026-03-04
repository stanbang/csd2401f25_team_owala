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

	enum ComponentType : int {
		TransformType = 0,
		VelocityType,
		AccelerationType,
		TriangleType,
		RigidBodyType,
		AABBColliderType,
		PlayerStateType,
		RotationFlagType,
		TrapPlatformComponentType,
		TextureComponentType,
		AudioComponentType,
		AnimationComponentType,
		TextComponentType,
		CameraType,
		NativeScriptsComponentType,
		PlayerTagType,
		FakePlayerTagType,
		ButtonType,
		PlatformTagType,
		BackgroundTagType,
		EnemyTagType,
		SceneOwnedType,
		PersistentTagType,
		LoadedTagType,
		FSMComponentType,
		GUITagType,
		SpawnPointTagType,
		ParticleComponentType,
		ParticleEmitterComponentType,
		SliderType,
		PlayerHealthComponentType,
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
	template <> struct ComponentTypeTrait<Transform> { static constexpr ComponentType value = TransformType; };
	template <> struct ComponentTypeTrait<Velocity> { static constexpr ComponentType value = VelocityType; };
	template <> struct ComponentTypeTrait<Acceleration> { static constexpr ComponentType value = AccelerationType; };
	template <> struct ComponentTypeTrait<Triangle> { static constexpr ComponentType value = TriangleType; };
	template <> struct ComponentTypeTrait<RigidBody> { static constexpr ComponentType value = RigidBodyType; };
	template <> struct ComponentTypeTrait<AABBCollider> { static constexpr ComponentType value = AABBColliderType; };
	template <> struct ComponentTypeTrait<PlayerState> { static constexpr ComponentType value = PlayerStateType; };
	template <> struct ComponentTypeTrait<RotationFlag> { static constexpr ComponentType value = RotationFlagType; };
	template <> struct ComponentTypeTrait<TrapPlatformComponent> { static constexpr ComponentType value = TrapPlatformComponentType; };
	template <> struct ComponentTypeTrait<TextureComponent> { static constexpr ComponentType value = TextureComponentType; };
	template <> struct ComponentTypeTrait<AudioComponent> { static constexpr ComponentType value = AudioComponentType; };
	template <> struct ComponentTypeTrait<AnimationComponent> { static constexpr ComponentType value = AnimationComponentType; };
	template <> struct ComponentTypeTrait<TextComponent> { static constexpr ComponentType value = TextComponentType; };
	template <> struct ComponentTypeTrait<Camera> { static constexpr ComponentType value = CameraType;};
	template <> struct ComponentTypeTrait<NativeScriptsComponent> { static constexpr ComponentType value = NativeScriptsComponentType; };
	template <> struct ComponentTypeTrait<PlayerTag> { static constexpr ComponentType value = PlayerTagType; };
	template <> struct ComponentTypeTrait<FakePlayerTag> { static constexpr ComponentType value = FakePlayerTagType; };
	template <> struct ComponentTypeTrait<Button> { static constexpr ComponentType value = ButtonType; };
	template <> struct ComponentTypeTrait<PlatformTag> { static constexpr ComponentType value = PlatformTagType; };
	template <> struct ComponentTypeTrait<BackgroundTag> { static constexpr ComponentType value = BackgroundTagType; };
	template <> struct ComponentTypeTrait<EnemyTag> { static constexpr ComponentType value = EnemyTagType; };
	template <> struct ComponentTypeTrait<SceneOwned> { static constexpr ComponentType value = SceneOwnedType; };
	template <> struct ComponentTypeTrait<PersistentTag> { static constexpr ComponentType value = PersistentTagType; };
	template <> struct ComponentTypeTrait<LoadedTag> { static constexpr ComponentType value = LoadedTagType; };  
	template <> struct ComponentTypeTrait<FSMComponent> { static constexpr ComponentType value = FSMComponentType; };
	template <> struct ComponentTypeTrait<ParticleComponent> { static constexpr ComponentType value = ParticleComponentType; };
	template <> struct ComponentTypeTrait<ParticleEmitterComponent> { static constexpr ComponentType value = ParticleEmitterComponentType; };
	template <> struct ComponentTypeTrait<Slider> { static constexpr ComponentType value = SliderType; };
	template <> struct ComponentTypeTrait<PlayerHealthComponent> { static constexpr ComponentType value = PlayerHealthComponentType; };
	template <> struct ComponentTypeTrait<GUITag> { static constexpr ComponentType value = GUITagType; };
	template <> struct ComponentTypeTrait<SpawnPointTag> { static constexpr ComponentType value = SpawnPointTagType; };

#pragma endregion

	using ComponentSignature = std::bitset<MAX_COMPONENTS>; // Bitset to track which components an entity has

	class EntityStorage{
	private:
		std::unordered_map<Entity, ComponentSignature> entitySignatures; // Track components of each entity
		friend class EntityManager; // Allow EntityManager to access private members
	private:
		/* Basic Components */
		std::unordered_map<Entity, EntityProperties> properties;
		std::unordered_map<Entity, Transform>	 transform;
		std::unordered_map<Entity, Velocity>     velocity;
		std::unordered_map<Entity, Acceleration> acceleration;
		std::unordered_map<Entity, Triangle>     triangle;
		std::unordered_map<Entity, RigidBody>    rigidBody;    
		std::unordered_map<Entity, AABBCollider> colliders;
		std::unordered_map<Entity, PlayerState> playerState;
		std::unordered_map<Entity, RotationFlag> rotationFlag;
		std::unordered_map<Entity, TrapPlatformComponent> trapPlatform;
		std::unordered_map<Entity, Button> button;

		std::unordered_map<Entity, TextureComponent>      tex; //Texture Component
		std::unordered_map<Entity, AudioComponent>        audio; //Audio Component
		std::unordered_map<Entity, AnimationComponent> animatedTex; //Animated Texture Component
		std::unordered_map<Entity, TextComponent>	text; // Text Component

		std::unordered_map<Entity, FSMComponent> fsmComponent; //fsm compoenent
		std::unordered_map<Entity, ParticleComponent> particleComponent;
		std::unordered_map<Entity, ParticleEmitterComponent> particleEmitterComponent;
		std::unordered_map<Entity, Slider> sliders;

		std::unordered_map<Entity, Camera> cameras;//Camera

		std::unordered_map<Entity, NativeScriptsComponent> nativeScripts; // Native Scripts
		std::unordered_map<Entity, PlayerHealthComponent> playerHealth;

		/* Scene Components */
		std::unordered_map<Entity, SceneOwned>		sceneOwned; // Tag to indicate which scene this entity belongs to
	public:
		/* Constructors */
		EntityStorage() = default;
		EntityStorage(const EntityStorage& other); // Copy Constructor
		EntityStorage& operator=(const EntityStorage & other); // Copy Assignment
		EntityStorage(EntityStorage&& other) noexcept = default; // Move Constructor
		EntityStorage& operator=(EntityStorage&& other) noexcept = default; // Move Assignment
		~EntityStorage();

		void SwapWith(EntityStorage& other) noexcept;
	};

	// BELOW IS STILL WIP. DO NOT USE.

	class WIPEntityStorage {
	private:
		// Archetypes is essentially a Set of components
		struct Archetype {
			ComponentSignature signature; // Components in this Archetype
			std::vector<Entity> entities; // Entities that belong to this Archetype

			std::vector<Velocity> velocities;
			std::vector<Acceleration> accelerations;
			std::vector<RigidBody> rigidBodies;
			std::vector<AABBCollider> aabbColliders;
			std::vector<Triangle> triangles;
			std::vector<TextureComponent> textures;
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