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
#include <memory> // for std::shared_ptr

namespace Framework{

	// Enum for defining physics layers
	enum class CollisionLayer : uint8_t {
		Default = 0,
		Player,
		Platform,
		Enemy,
		Trigger,
		Count // track the number of layers
	};

	// Enum for animation states
	enum class AnimationState {
		IDLE,
		RUNNING,
		JUMPING,
		FALLING
	};

	// Main Data Component that should exist in all objects. Stores core attributes for the entity. THIS IS ALWAYS ON A GAMEOBJECT.
	struct EntityProperties {
		std::string name = "Empty";
		CollisionLayer layer = CollisionLayer::Default;
	};

	/*	Position component to store x and y 
		coordinates of an entity.*/
	struct Position {
		Vector2D Pos;
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
		Vector2D HalfSize; // Half-width and half-height of the box
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
	};

	struct TextureComponent {
		std::string textureKey; // Key to identify the texture resource
		Vector2D HalfSize = { 1, 1 };
	};

	struct AudioComponent {
		std::string soundKey;
		bool loop = false;
		bool stream = false;
		float volume = 1.0f;
	};

	struct AnimationComponent {
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
		std::string textToDraw;
		std::string fontName;
		Color color = { 1,1,1,1 };
		float scale = 1.0f;
	};

	struct PlayerStats {
		float moveSpeed = 0;
		Position pos;
	};;

	/* TAG SECTION */
	/* Used for ID-ing an entity */
	struct PlayerTag {};
	struct FakePlayerTag {};
	struct PlatformTag {};
	struct BackgroundTag {};
	struct EnemyTag {};

	struct RotationFlag {
		
		bool isRotating = false;
		float currentRotation = 0.0f;
		float targetRotation = 0.0f;
	};

	


	// Component to track trap platform state
	struct TrapPlatformComponent {
		bool isTriggered = false;
		bool hasFallen = false;
		float triggerDelay{ 0.0f };
		float currentDelay{ 0.0f };
		float fallSpeed{ 0.0f };
		Vector2D originalPosition;
		float currentFallDistance{ 0.0f };
	};

	struct ScalingPlatformComponent {
		Vector2D baseHalfSize;
		float currentScaleX{ 0.0f };
		float currentScaleY{ 0.0f };
		float scaleSpeed{ 0.0f };
		float minScale{ 0.0f };
		float maxScale{ 0.0f };
		float time{ 0.0f };
		Matrix3x3 transformMatrix;
	};

}