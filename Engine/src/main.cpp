#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"

int main() {
	Framework::CoreEngine engine;

	// ---- pick ONE of the two AddSystem patterns below ----
	// (A) If Core.h has RAW-POINTER AddSystem(ISystem*)
	auto* win = new Framework::WindowsSystem(engine, "Owala", 800, 600);
	engine.AddSystem(win);
	engine.AddSystem(new Framework::Graphics(*win));

	// (B) If Core.h has TEMPLATE AddSystem<T>(...)
	// auto* win = engine.AddSystem<Framework::WindowsSystem>(engine, "Owala", 800, 600);
	// engine.AddSystem<Framework::Graphics>(*win);

	engine.Initialize();
	engine.GameLoop();
	engine.DestroySystems();
	return 0;
}
