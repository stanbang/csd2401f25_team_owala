#pragma once
#include "Entity.h"

namespace Framework {

	enum class ButtonAction{
	DEFAULT,
	PLAYSOUND,
	RESETPLAYERPOS
	};

	struct PlayerJumpedEvent{
		Entity player;
		float damage; //just for example, currently not used yet
		float volume = 1.0f;
	};

	struct ButtonClickedEvent {
		Entity button;
		ButtonAction action;
	};


} // namespace Framework
