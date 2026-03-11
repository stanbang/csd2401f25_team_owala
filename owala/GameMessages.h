/*****************************************************************//**
 * \file   GameMessages.h
 *
 * \primary_author
 *      Ng Jing Wen (70%)
 * \secondary_author
 *		Ng Jordain Ying Zhe (30%)
 *
 * \brief	Declarations of various game event message structs for 
 *			inter-system communication.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "Entity.h"
#include "Vector2D.h"
#include <string>

namespace Framework {

	// Separates different audio channel groups
	enum class AudioType {
		SFX,
		MUSIC
	};

	// Generic audio trigger events — consumed by AudioSystem, published by any game/engine system
	struct PlaySoundEvent {
		std::string soundKey;
		AudioType   audioType = AudioType::SFX;
		float       volume    = 1.0f;
	};

	struct PlaySound3DEvent {
		std::string soundKey;
		AudioType   audioType = AudioType::SFX;
		float       volume    = 1.0f;
		Vector2D    position;
		float       minDist   = 50.0f;
		float       maxDist   = 700.0f;
	};

	enum class ButtonAction{
		DEFAULT,
		PLAYSOUND,
		RESETPLAYERPOS,
		SWITCH_SCENE,
		SWITCH_SCENE_ADD,
		QUIT_GAME,
		PREV_PAGE,
		NEXT_PAGE,
		RESUME_GAME
	};

	// Enum for different particle movement effects
	enum class EffectType {
		NONE,
		DRIFT,
		RANDOM
	};

	// Enum for different emission styles
	enum class EmissionStyle {
		FROM_POINT,           // Emit from the position component
		FROM_RADIUS,          // Emit randomly within a radius around position
		FROM_TOP_VIEWPORT,    // Emit from top of screen falling down, still feels wonky
		FROM_LEFT_VIEWPORT,   // Emit from left side, moving right, still feels wonky
		FROM_RIGHT_VIEWPORT,  // Emit from right side, moving left, still feels wonky
		FROM_BOTTOM_VIEWPORT, // Emit from bottom, moving up, still feels wonky
		FROM_LINE,            // Emit along a line
		FROM_CIRCLE_EDGE      // Emit from the edge of a circle (WIP)
	};

	struct PlayerJumpedEvent{
		Entity player;
		float damage; //just for example, currently not used yet
	};

	struct PlayerDeathEvent {
		Entity player;
		bool deathAnimationFinished = false;
	};

	struct ButtonClickedEvent {
		Entity button;
		ButtonAction action;

		// Data payloads
		std::string stringParam; // Scene Name, Audio Key etc
		float floatParam;
		std::string sceneToUnload; // Scene to unload when this button fires (may be empty)
	};

	struct ParticleBurstEvent {
		Vector2D position;              // Where to spawn
		int count = 1;                 // How many particles in the burst
		bool isAnimated = false;		// Is the particle animated
		std::string textureKey;         // Texture to use

		// Optional overrides
		float speed = 1.0f;
		float lifetime = 1.0f;
		EffectType effect = EffectType::NONE;
		float effectStrength = 1.0f;
		Vector2D size = { 0.1f, 0.1f };
		bool isScreenSpace = false;

		EmissionStyle style = EmissionStyle::FROM_POINT;
		float radius = 0.5f; // Used if style is FROM_RADIUS or CIRCLE_EDGE
		Vector2D offset = { 0.0f, 0.0f };

		bool isGlobal = true;
	};

	struct VFXTriggerEvent {
		std::string triggerName; 

		bool overridePosition = false; // If true, use position below
		Vector2D position = { 0.0f, 0.0f };
	};

	// Events to publish during specific animation frames, e.g. to trigger SFX or VFX at the right timing
	struct AnimationEvent {
		Entity entity;
		std::string eventName;
	};

	struct TrapPlatformLandEvent {
		Vector2D position;
	};

	// Empty structs for signalling
	struct LevelClearEvent {};
	struct LevelStartEvent {};

	struct WorldRotationEvent {};
	struct PlatformLandEvent {};

	struct PlayerDamagedEvent {
		Entity playerEntity;
		int livesRemaining;
	};
	struct PlayerLandEvent {};
	struct CurtainsOpenEvent {};
	struct CurtainsCloseEvent {};

	struct BirdAttackEvent {
		Entity entity;
		Vector2D position;
	};
	struct BirdWakeEvent {
		Entity entity;
		Vector2D position;
	};

	// Generic engine-level event: any entity has landed on a surface.
	// Game systems (TrapPlatformSystem, etc.) subscribe to this instead of
	// having physics publish game-specific events.
	struct EntityLandedEvent {
		Entity   entity;
		Vector2D position;
	};

} // namespace Framework
