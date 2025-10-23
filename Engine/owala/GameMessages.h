#pragma once
#include "Entity.h"

namespace Framework {

	struct PlayerJumpedEvent{
		Entity player;
		float damage; //just for example, currently not used yet
	};

} // namespace Framework
