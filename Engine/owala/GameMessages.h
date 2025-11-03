#pragma once
#include "Entity.h"

namespace Framework {

	struct PlayerJumpedEvent{
		Entity player;
		float damage; //just for example, currently not used yet
		float volume = 1.0f;
	};

	struct ButtonClickedEvent {
		Entity button;
	};


} // namespace Framework
