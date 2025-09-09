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
