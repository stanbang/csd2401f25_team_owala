#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "GameSystem.h"

int main() {
	Framework::CoreEngine engine;

	auto* win = new Framework::WindowsSystem(engine, "Owala", 800, 600);
	engine.AddSystem(win);                         // window + GL context

	auto* gfx = new Framework::Graphics(*win);
	engine.AddSystem(gfx);                         // renderer

	engine.AddSystem(new Framework::GameSystem(engine, *gfx)); // your team’s code

	engine.Initialize();
	engine.GameLoop();
	engine.DestroySystems();
	return 0;
}
/* ISystem exists to give your engine a single, simple contract that every subsystem 
follows—Initialize(), Update(dt), and Shutdown()—so the core loop can treat all parts 
(windowing, graphics, input, physics, gameplay, audio) uniformly without knowing their details. 
By storing a list of ISystem*, the CoreEngine can iterate and call the same three methods on each
system (polymorphism), which cleanly decouples what the systems do from when they run. 
This makes the architecture plug-and-play: you can add, remove, or reorder systems without
changing the core, control execution order by the add sequence, and keep each system responsible
for its own resources and lifecycle. It also improves testability and teamwork—frontend/gameplay
folks can write a GameSystem that only calls high-level APIs (like drawTriangle) while backend
folks evolve Graphics or Physics behind the same interface—so the engine stays modular, 
predictable, and easy to extend.*/