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

namespace Framework {

	enum class ButtonAction{
		DEFAULT,
		PLAYSOUND,
		RESETPLAYERPOS,
		SWITCH_SCENE,
		QUIT_GAME
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
		float volume = 1.0f;
	};

	struct ButtonClickedEvent {
		Entity button;
		ButtonAction action;

		// Data payloads
		std::string stringParam; // Scene Name, Audio Key etc
		float floatParam;
	};

	struct PlayerDeathEvent {
		Entity player;
		bool deathAnimationFinished = false;
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
	};

} // namespace Framework
