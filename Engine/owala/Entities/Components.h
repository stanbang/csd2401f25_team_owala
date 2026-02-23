/*****************************************************************//**
 * \file   Components.h
 *
 * \primary_author
 *      Ng Jing Wen (16%)
 * \secondary_author
 *		Lur Pei Hao Stanley (16%)
 *      Sitara Anil (16%)
 *      Lou Jason (16%)
 *      Ng Jing Wen (16%)
 *      Yar Xuan (16%)
 *
 * \brief	Contains all component definitions.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "Graphics.h"
#include "Vector2D.h"
#include <string>
#include "Matrix3x3.h"
#include "GameMessages.h"
#include <memory> // for std::shared_ptr
#include <vector> 
#include <map>
#include "ScriptableEntity.h"
#include "FSM.h"

namespace Framework{

	// Enum for defining physics layers
	enum class CollisionLayer : uint8_t {
		Default = 0,
		Player,
		Platform,
		Enemy,
		Trigger,
		TrapPlatform,    
		Invulnrabilty, 
		Count = 32  // Maximum layers 
	};

	// Enum for defining rendering layers 
	enum class RenderLayer : int {
		Background = -100,
		Platform = 50,
		Enemy = 75,
		Player = 100,
		UI = 200,
		Custom = 0  // For entities with custom render order
	};

	// Enum for animation states
	enum class AnimationState {
		IDLE,
		WAKING,
		RUNNING,
		JUMPING,
		FALLING,
		DEATH,
		HURT
	};

	// For particle emission modes
	enum class ParticleEmissionMode {
		CONTINUOUS,    // Emit constantly while active
		BURST,         // Emit once on trigger
		LOOP_BURST     // Emit multiple bursts on trigger
	};

	// To separate different audio types
	enum class AudioType {
		SFX,
		MUSIC
	};

	// Different slider sound types
	enum class SliderSoundType {
		MASTER_VOLUME,
		MUSIC_VOLUME,
		SFX_VOLUME
	};

	// Main Data Component that should exist in all objects. Stores core attributes for the entity. THIS IS ALWAYS ON A GAMEOBJECT.
	struct EntityProperties {
		std::string name = "Empty";
		CollisionLayer layer = CollisionLayer::Default;
		std::string collisionLayerName = "";
		RenderLayer renderLayer = RenderLayer::Platform;
		int customRenderOrder = 0; // Used when renderLayer is Custom
		std::string renderLayerName = ""; // store name of dynamiclly created layers
		bool isActive = true;

		uint64_t uuid;
	};

	/* Transform Component indicating data relating to the entity's Position, Rotation and Scale */
	struct Transform {
		struct Rotation {
			float angle = 0.0f;
			float textureAngle = 0.0f; // Texture rotation angle
			float colliderAngle = 0.0f; // Collider rotation angle
			Matrix3x3 textureTransformMatrix; // For texture
			Matrix3x3 colliderTransformMatrix; // For collider

			Rotation() : textureAngle(0.0f), colliderAngle(0.0f), textureTransformMatrix(), colliderTransformMatrix() {}
			Rotation(float texAngle, float colAngle = 0.0f) : textureAngle(texAngle), colliderAngle(colAngle), textureTransformMatrix(), colliderTransformMatrix() {}
		};

		Vector2D position{ 0.f, 0.f };
		Rotation rotation;
		Vector2D scale{ 1.f, 1.f };

		// Parameterized Constructor
		Transform(const Vector2D& pos = Vector2D{ 0.f, 0.f }, float angle = 0.f, const Vector2D& scl = Vector2D{ 1.f, 1.f })
			: position(pos), rotation(angle), scale(scl) {}
	};

	/*	Velocity component to store x and y
		velocity of an entity.*/
	struct Velocity {
		Vector2D Vel;
	};

	/* Rigidbody component for physics simulation properties. */
	struct RigidBody {
		Vector2D Force;
		bool isAffectedByGravity = true;
		float Mass = 1.0f;
		float InverseMass = 1.0f;

		void SetMass(float mass) {
			Mass = mass;
			InverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
		}
	};

	/* AABB (Axis-Aligned Bounding Box) for collision detection. */
	struct AABBCollider {
		Vector2D HalfSize{ 50.0f, 50.0f }; // Half-width and half-height of the box
		Vector2D Offset{ 0.0f, 0.0f };     // Offset from entity position
		float rotation = 0.0f;
		Matrix3x3 transformMatrix;

		AABBCollider() : HalfSize(0.0f, 0.0f), Offset(0.0f, 0.0f), rotation(0.0f), transformMatrix() {}
		AABBCollider(const Vector2D& size) : HalfSize(size), Offset(0.0f, 0.0f), rotation(0.0f), transformMatrix() {}
		AABBCollider(const Vector2D& size, const Vector2D& offset) : HalfSize(size), Offset(offset), rotation(0.0f), transformMatrix() {}
	};
	

	/*	Acceleration component to store x and y 
		acceleration of an entity.*/
	struct Acceleration {
		Vector2D Accel;
	};

	// A component to store player-specific states
	struct PlayerState {
		bool isFacingRight = true; // Direction the player is facing
		bool isGrounded = false; // Flag for checking if player is grounded on floor
		bool isWalking = false;

		float previousVelocityY = 0.0f; // Store previous frame's Y velocity
		bool isDead = false;

		int currentLives = 3;  // Start with 3 lives
		bool isHurt = false;
		float invulnerabilityTimer = 0.0f;

		bool isKnockedBack = false;
		float knockbackTimer = 0.0f;
	};

	struct Triangle { 
		Vector2D a, b, c; 
		Color color = { 1,1,1,1 };
	};

	/* Camera component for view control and transformation */
	struct Camera {
		Vector2D position = { 0.0f, 0.0f };  // World position camera is looking at
		Vector2D size = { 2.0f, 2.0f };      // How much world space is visible (width, height)
		float zoom = 1.0f;                 // Zoom level (1.0 = normal)
		float aspectRatio = 16.0f / 9.0f; // Add this
		uint32_t targetEntity = 0;        // Use uint32_t instead
		float followSpeed = 5.0f;          // How quickly camera follows target (0 = instant)
		bool isActive = false;             // Main camera flag
		bool isEditorControlled = false;   // NEW: Prevent game from moving this camera
	
		bool useMapBounds = false;
		float mapMinX = -100.0f;
		float mapMaxX = 100.0f;
		float mapMinY = -100.0f;
		float mapMaxY = 100.0f;

		bool zoomInitialized = false;  // ADD THIS LINE

	};

	struct TextureComponent {
		std::string textureKey; // Key to identify the texture resource
		Vector2D HalfSize = { 50.f, 50.f };
		bool flipHorizontal = false;
	};

	struct AudioComponent {
		std::string soundKey;
		bool loop = false;
		bool stream = false;
		float volume = 0.5f;
		AudioType audioType = AudioType::SFX; // Default audio is SFX
	};

	struct AnimationComponent {
		std::string profileName; // Name of the animation profile to use(can be Player, BirdEnemy etc)
		AnimationState currentState = AnimationState::IDLE; // Default animation state is IDLE

		int currentFrame = 0;      // Frame index
		float frameDuration = 0.1f;
		int startFrame = 0;        // which frame to start the animation from
		int frameCount = 1;        // how many frames this animation has
		float elapsedTime = 0.0f; // Time elapsed since the last frame change
		bool isPlaying = true; // Is the animation currently playing
		bool isLooping = true; // Should the animation loop when it reaches the end

		Vector2D minUV = { 0.0f, 0.0f }; // Top-Left UV of the current frame
		Vector2D maxUV = { 1.0f, 1.0f }; // Bottom-Right UV of the current frame
	};

	struct TextComponent {
		std::string textToDraw{};
		std::string fontKey{};
		Color color = { 1,1,1,1 };
		float scale = 1.0f;
		float maxWidth = 10.0f;

		// no need to serialize, calculated by text render system 
		Vector2D halfSize = { 0.0f, 0.0f };
		std::vector<std::string> wrappedLines;
		bool isDirty = true;
	};

	/* TAG SECTION */
	/* Used for ID-ing an entity */
	struct PlayerTag {};
	struct FakePlayerTag {};
	struct PlatformTag {};
	struct BackgroundTag {};
	struct EnemyTag {};
	struct GUITag {};

	/*	SpawnPoint tag for player spawn point*/
	struct SpawnPointTag {};

	struct RotationFlag {
		
		bool isRotating = false;
		float currentRotation = 0.0f;
		float targetRotation = 0.0f;
		Matrix3x3 transformMatrix;
	};

	// Component to track trap platform state
	struct TrapPlatformComponent {
		bool isTriggered = false;
		bool hasFallen = false;
		bool isGrounded = false;
		float triggerDelay{ 0.0f };
		float currentDelay{ 0.0f };
		float fallSpeed{ 0.0f };
		float airTime{ 0.0f };
		Vector2D originalPosition;
		//float currentFallDistance{ 0.0f };
	};

	struct Button {
		bool isHovered = false; // Mouse is over the button
		bool isPressed = false; // Button is clicked
		bool isReleased = false; // Button click is released
		ButtonAction action = ButtonAction::DEFAULT; // Action to perform on click

		// Data to customize the action
		std::string targetString = ""; // Stores Scene Name or Sound File
		float targetValue = 0.0f;      // Stores specific numbers if needed
	
		// Sound effects
		std::string hoverSoundKey = "";   // Sound to play when hovering
		float hoverSoundVolume = 0.5f;    // Volume for hover sound
		std::string clickSoundKey = "";   // Sound to play when clicked
		float clickSoundVolume = 0.5f;    // Volume for click sound

		bool wasHoveredLastFrame = false; // Track hover state changes
	
		// Texture keys for different button states
		std::string defaultTextureKey = "";   // Default/idle state
		std::string hoverTextureKey = "";     // When hovering
		std::string pressedTextureKey = "";   // When pressed

		// Track which textures are animated
		bool defaultIsAnimated = false;
		bool hoverIsAnimated = false;
		bool pressedIsAnimated = false;

		// NEW: Track current state for texture switching
		enum class ButtonVisualState {
			DEFAULT,
			HOVER,
			PRESSED
		};
		ButtonVisualState currentVisualState = ButtonVisualState::DEFAULT;
	};

	struct NativeScriptsComponent {
		struct NativeScript {
			ScriptableEntity* Instance = nullptr;

			// Function Pointers
			ScriptableEntity* (*InstantiateScript)() = nullptr;
			void (*DestroyScript)(NativeScript*) = nullptr;

			// Metadata
			const char* scriptName = nullptr;
			std::string serializedTypeName;
			bool missing = false;
			bool enabled = true;
			bool executeInEditMode = false;

			template<typename T>
			void Bind(const char* name) {
				static_assert(std::is_base_of<ScriptableEntity, T>::value, "T must derive from ScriptableEntity");

				InstantiateScript = +[]() -> ScriptableEntity* { return new T(); };
				DestroyScript = +[](NativeScript* script) { delete script->Instance; script->Instance = nullptr; };

				scriptName = name;
				serializedTypeName = name;
				missing = false;
			}

			void Reset() noexcept {
				if (!Instance) return;
				if (DestroyScript) DestroyScript(this);
				else { delete Instance; Instance = nullptr; }
				Instance = nullptr;
			}

			void FlagMissing(const std::string& jsonTypeName) {
				Reset();
				InstantiateScript = nullptr;
				DestroyScript = +[](NativeScript* script) { delete script->Instance; script->Instance = nullptr; };
				scriptName = nullptr;
				serializedTypeName = jsonTypeName;
				missing = true;
				enabled = executeInEditMode = false;
			}

			// BELOW IS USING RULE OF 3. WILL CONVERT TO RAII IN THE FUTURE. - Jason
			/* Constructors */
			NativeScript() noexcept = default;
			~NativeScript() { Reset(); }
			
			// Copy Ctor
			NativeScript(const NativeScript& other) : Instance(nullptr), InstantiateScript(other.InstantiateScript), DestroyScript(other.DestroyScript), 
				scriptName(other.scriptName), serializedTypeName(other.serializedTypeName), missing(other.missing), enabled(other.enabled),
				executeInEditMode(other.executeInEditMode) {}

			// Copy Assignment
			NativeScript& operator=(const NativeScript& other) {
				if (this == &other) return *this;

				Reset();
				InstantiateScript = other.InstantiateScript;
				DestroyScript = other.DestroyScript;
				scriptName = other.scriptName;
				serializedTypeName = other.serializedTypeName;
				missing = other.missing;
				enabled = other.enabled;
				executeInEditMode = other.executeInEditMode;
				Instance = nullptr; // Should not be copied
				return *this;
			}

			// Move Ctor
			NativeScript(NativeScript&& other) noexcept	: Instance(other.Instance), InstantiateScript(other.InstantiateScript), DestroyScript(other.DestroyScript), 
				scriptName(other.scriptName), serializedTypeName(std::move(other.serializedTypeName)), missing(other.missing), enabled(other.enabled),
				executeInEditMode(other.executeInEditMode) 
			{
				other.Instance = nullptr;
				other.InstantiateScript = nullptr;
				other.DestroyScript = nullptr;
				other.scriptName = nullptr;
				other.missing = false;
				other.enabled = true;
				other.executeInEditMode = false;
			}

			// Move Assignment
			NativeScript& operator=(NativeScript&& other) noexcept {
				if (this != &other) {
					Reset();
					Instance = other.Instance;
					InstantiateScript = other.InstantiateScript;
					DestroyScript = other.DestroyScript;
					scriptName = other.scriptName;
					serializedTypeName = std::move(other.serializedTypeName);
					missing = other.missing;
					enabled = other.enabled;
					executeInEditMode = other.executeInEditMode;

					other.Instance = nullptr;
					other.InstantiateScript = nullptr;
					other.DestroyScript = nullptr;
					other.scriptName = nullptr;
					other.serializedTypeName.clear();
					other.missing = false;
					other.enabled = true;
					other.executeInEditMode = false;
				}
				return *this;
			}
		};

		std::vector<NativeScript> scripts;
	};
	
	/**
	 * @struct FSMComponent
	 * @brief Data-driven FSM container.
	 */
	struct FSMComponent {
		// Serialized Data
		std::string fsmDefinitionPath;  // e.g. "Assets/Config/BirdAI.json"
		std::string initialStateOverride;

		// Runtime Data (Do not serialize the whole FSM object)
		Framework::FSM fsm;
		bool initialized = false;

		// Used during deserialization to restore state
		std::string serializedState;

		// Visual effect: active attack circle radius (0 = none)
		float attackCircleRadius = 0.0f;
	};

	// Particle Component
	struct ParticleComponent {
		EffectType effectType = EffectType::NONE; // Type of effect the particle will have
		float lifetime = 1.0f;      // Total lifetime of the particle
		float age = 0.0f;           // Current age of the particle
		bool isScreenSpace = false; // Is the particle rendered in screen space
	};

	struct ParticleEmitterComponent {
		bool isEmitting = true;
		bool isParticleAnimated = false; // Is the particle texture animated
		bool isScreenSpace = false;   // Are the particles rendered in screen space
		ParticleEmissionMode emissionMode = ParticleEmissionMode::CONTINUOUS;

		// For BURST/LOOP_BURST modes
		std::string triggerEventName;  // Name of the event to listen for
		int burstCount = 1;            // How many bursts to emit (for LOOP_BURST)
		float burstInterval = 0.5f;    // Time between bursts (for LOOP_BURST)
		float timeSinceBurst = 0.0f;   // Internal timer for burst spacing

		std::string particleTextureKey; // Key to identify the texture resource for particles
		float particleLifetime = 1.0f;	// Total lifetime of the particle
		float emissionRate = 3.0f;		// Particles emitted per second
		float emissionInterval = 0.0f;	// Time between emissions (calculated from emissionRate)
		float timeSinceLastEmission = 0.0f; // Time since the last particle was emitted
		EffectType particleEffectType = EffectType::NONE; // Type of effect for emitted particles
		float effectStrength = 1.0f; // Strength of the effect
		float particleSpeed = 1.0f;	// Speed of emitted particles
		Vector2D particleSize = { 1.0f, 1.0f }; // Size of emitted particles

		Vector2D particleStartVelocity = { 0.0f, 0.0f };// Initial velocity of emitted particles
		Vector2D particleAcceleration = { 0.0f, 0.0f }; // For moving particles
		bool isAffectedByGravity = false;				// Should particles be affected by gravity

		EmissionStyle emissionStyle = EmissionStyle::FROM_POINT; // Style of emission
		Vector2D emissionOffset = { 0.0f, 0.0f }; // Offset from emitter position

		// For FROM_RADIUS style
		float emissionRadius = 0.5f;		// Radius around position to emit particles

		// For FROM_LINE style
		Vector2D lineStart = { -1.0f, 0.0f };   // Start point of emission line
		Vector2D lineEnd = { 1.0f, 0.0f };     // End point of emission line

		// For FROM_CIRCLE_EDGE style
		float circleRadius = 0.5f;              // Radius of the circle

		// For screen space: use normalized coordinates (0 to 1)
		// For world space: use world coordinates
		Vector2D viewportMin = { 0.0f, 0.0f };
		Vector2D viewportMax = { 1.0f, 1.0f };


		// Direction override
		Vector2D emissionDirection = { 0.0f, -1.0f }; // Default downward for top emission
	};

	struct Slider {
		Entity handleEntity = 0;
		std::string handleName = "";

		float minValue = 0.0f;
		float maxValue = 1.0f;
		float currentValue = 1.0f;
		float width = 1.0f; // Total width of the slider bar
		float widthOffsetX = 0.0f;
		bool isDragging = false;
		bool needsResolution = true;

		SliderSoundType sliderType = SliderSoundType::MASTER_VOLUME; //Default to master volume
		bool isInitialized = false;
	};

	struct PlayerHealthComponent {
		int lifeNum{};
	};

}