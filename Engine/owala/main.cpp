#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "GameSystem.h"
#include "InputSystem.h"

int main() {
    Framework::CoreEngine engine;

    Framework::WindowsSystem *win = new Framework::WindowsSystem(engine, "Owala", 800, 600);
    Framework::InputSystem *input = new Framework::InputSystem(*win);
    Framework::EntityStorage *ecs = new Framework::EntityStorage();
    Framework::Graphics *gfx = new Framework::Graphics(*win);
    Framework::GameSystem *game = new Framework::GameSystem(*ecs, *gfx, *win, *input);

    engine.AddSystem(win);                 // creates GLFW window + context
    engine.AddSystem(input);               // MUST come before GameSystem
    engine.AddSystem(game);                // reads input -> moves/queues draws
    engine.AddSystem(gfx);                 // swaps buffers last

    engine.Initialize();
    engine.GameLoop();
    engine.DestroySystems();
    return 0;
}
