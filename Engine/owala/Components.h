/*****************************************************************//**
 * \file   Components.h
 * \brief  All components used for entities and the ECS architecture
 *		   are stored here.
 * 
 * \author Ng Jing Wen
 * \date   9 September 2025
 *********************************************************************/
#pragma once
#include "Graphics.h"
#include "Vector2D.h"

namespace Framework{
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
		/*float xAccel;
		float yAccel;*/
		Vector2D Accel;
	};

	struct Triangle { 
		Vector2D a, b, c; 
		Color color; 
	};

	struct PlayerStats {
		float moveSpeed;
	};;

	/* TAG SECTION */
	/* Used for ID-ing an entity */
	struct PlayerTag {};
	struct PlatformTag {};
	struct EnemyTag {};
	struct RotationFlag {
		//bool shouldRotate = false;
		bool isRotating = false;
		float currentRotation = 0.0f;
		float targetRotation = 0.0f;
	};

	// A component to store player-specific states
	struct PlayerState {
		bool isGrounded = false;
	};


}


