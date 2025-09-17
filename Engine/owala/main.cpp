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
#include <iostream>

int main() {
    Framework::CoreEngine engine;
	Framework::FileIOSystem fileIO;
	Vector2D windowConfig = fileIO.loadWindowConfig();
    Framework::WindowsSystem *win = new Framework::WindowsSystem(engine, "Owala", windowConfig.x, windowConfig.y);
    Framework::InputSystem *input = new Framework::InputSystem(*win);
    Framework::EntityStorage *ecs = new Framework::EntityStorage();
    Framework::PhysicsSystem* phx = new Framework::PhysicsSystem(*ecs);
    Framework::RotationSystem* rotation = new Framework::RotationSystem(*ecs, *input, *phx);
    Framework::PlayerSystem* player = new Framework::PlayerSystem(*ecs, *input ,*phx);
    Framework::Graphics *gfx = new Framework::Graphics(*win);
    Framework::GameSystem *game = new Framework::GameSystem(*ecs, *gfx, *win, *input, *phx);

    //    This is the core logic loop: Input -> Physics -> Gameplay -> Rendering.
    engine.AddSystem(win);                 // creates GLFW window + context
    engine.AddSystem(input);               // MUST come before GameSystem
    engine.AddSystem(rotation);
    engine.AddSystem(player);              // PlayerSystem processes input after InputSystem.
    engine.AddSystem(phx);                 // Physics comes after input
    engine.AddSystem(game);                // reads input -> moves/queues draws
    engine.AddSystem(gfx);                 // swaps buffers last

    engine.Initialize();
    engine.GameLoop();
    engine.DestroySystems();
    return 0;
}
