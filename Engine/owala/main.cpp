#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "GameSystem.h"
#include "InputSystem.h"
#include "PlayerSystem.h"
#include "PhysicsSystem.h"
#include "FileUtility.h"
#include "RotationSystem.h"
#include "ResourceManager.h"
#include <iostream>

int main() {
    using namespace Framework;
    CoreEngine engine;
	FileIOSystem fileIO;
	Vector2D windowConfig = fileIO.loadWindowConfig();
    WindowsSystem *win = new WindowsSystem(engine, "Owala", (int)windowConfig.x, (int)windowConfig.y);
    InputSystem *input = new InputSystem(*win);
    EntityStorage *ecs = new EntityStorage();
    PhysicsSystem* phx = new PhysicsSystem(*ecs);
    RotationSystem* rotation = new RotationSystem(*ecs, *input, *phx);
    PlayerSystem* player = new PlayerSystem(*ecs, *input ,*phx);
    Graphics *gfx = new Graphics(*win);
    ResourceManager* rcm = new ResourceManager(*gfx); //Create resource manager on stack
    GameSystem *game = new GameSystem(*rcm, *ecs, *gfx, *win, *input, *phx);

    //    This is the core logic loop: Input -> Physics -> Gameplay -> Rendering.
    engine.AddSystem(win);                 // creates GLFW window + context
    engine.AddSystem(input);               // MUST come before GameSystem
    engine.AddSystem(rotation);
    engine.AddSystem(player);              // PlayerSystem processes input after InputSystem.
    engine.AddSystem(phx);                 // Physics comes after input
	engine.AddSystem(rcm);                // Resource manager (loads resources on demand) MUST come before GameSystem or assets won't be found
    engine.AddSystem(game);                // reads input -> moves/queues draws
    engine.AddSystem(gfx);                 // swaps buffers last

    engine.Initialize();
    engine.GameLoop();
    engine.DestroySystems();
    return 0;
}
