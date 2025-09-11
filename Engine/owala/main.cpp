#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "GameSystem.h"
#include "InputSystem.h"

int main() {
    Framework::CoreEngine engine;

    auto* win = new Framework::WindowsSystem(engine, "Owala", 800, 600);
    engine.AddSystem(win);                 // creates GLFW window + context

    auto* input = new Framework::InputSystem(*win);
    engine.AddSystem(input);               // MUST come before GameSystem

    auto* ecs = new Framework::EntityStorage();

    auto* gfx = new Framework::Graphics(*win);

    auto* game = new Framework::GameSystem(*ecs, *gfx, *win, *input);
    engine.AddSystem(game);                // reads input -> moves/queues draws

    engine.AddSystem(gfx);                 // swaps buffers last

    engine.Initialize();
    engine.GameLoop();
    engine.DestroySystems();
    return 0;
}
