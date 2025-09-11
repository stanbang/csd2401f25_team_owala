#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "GameSystem.h"
#include "PlayerSystem.h"
#include "FileUtility.h"
#include <iostream>

int main() {
	Framework::CoreEngine engine;
	Framework::FileIOSystem fileIO;

	// 1) Window + GL context
	Vector2D windowConfig = fileIO.loadWindowConfig();
	auto* win = new Framework::WindowsSystem(engine, "StageBound", windowConfig.x, windowConfig.y);
	engine.AddSystem(win);

	// 2) ECS
	auto* ecs = new Framework::EntityStorage();

	// 3) Gameplay (spawns + input + issues draw calls)
	auto* gfx = new Framework::Graphics(*win);
	engine.AddSystem(new Framework::GameSystem(*ecs, *gfx, *win));

	auto* playerSys = new Framework::PlayerSystem(*ecs);
	engine.AddSystem(playerSys);

	// 4) Graphics last (does buffer swap after GameSystem drew)
	engine.AddSystem(gfx);

	engine.Initialize();
	engine.GameLoop();
	engine.DestroySystems();
	return 0;
}


/* ISystem exists to give your engine a single, simple contract that every subsystem
follows�Initialize(), Update(dt), and Shutdown()�so the core loop can treat all parts
(windowing, graphics, input, physics, gameplay, audio) uniformly without knowing their details.
By storing a list of ISystem*, the CoreEngine can iterate and call the same three methods on each
system (polymorphism), which cleanly decouples what the systems do from when they run.
This makes the architecture plug-and-play: you can add, remove, or reorder systems without
changing the core, control execution order by the add sequence, and keep each system responsible
for its own resources and lifecycle. It also improves testability and teamwork�frontend/gameplay
folks can write a GameSystem that only calls high-level APIs (like drawTriangle) while backend
folks evolve Graphics or Physics behind the same interface�so the engine stays modular,
predictable, and easy to extend.*/