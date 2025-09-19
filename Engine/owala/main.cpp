#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "GameSystem.h"
#include "InputSystem.h"
#include "PlayerSystem.h"
#include "PhysicsSystem.h"
#include "FileUtility.h"
#include "SceneManager.h"
#include "RotationSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "Logger.h"

int main() {
    using namespace Framework;
    try {
        CoreEngine engine;
	    FileIOSystem fileIO;
	    Vector2D windowConfig = fileIO.loadWindowConfig();
        WindowsSystem *win = new WindowsSystem(engine, "Owala", (int)windowConfig.x, (int)windowConfig.y);
        InputSystem *input = new InputSystem(*win);
        EntityStorage *ecs = new EntityStorage();
        PhysicsSystem* phx = new PhysicsSystem(*ecs);
        SceneManager::GetInstance().SetEntityStorageRef(*ecs); // Creates SM Instance and sets the entity storage reference for the SceneManager
        RotationSystem* rotation = new RotationSystem(*ecs, *input, *phx);
        PlayerSystem* player = new PlayerSystem(*ecs, *input ,*phx);
        AudioSystem* audio = new AudioSystem();
        Graphics *gfx = new Graphics(*win);
        ResourceManager* rcm = new ResourceManager(*gfx); //Create resource manager on stack
        GameSystem *game = new GameSystem(*rcm, *ecs, *gfx, *win, *input, *phx, *audio);

        //    This is the core logic loop: Input -> Physics -> Gameplay -> Rendering.
        engine.AddSystem(win);                 // creates GLFW window + context
        engine.AddSystem(input);               // MUST come before GameSystem
        engine.AddSystem(rotation);
        engine.AddSystem(player);              // PlayerSystem processes input after InputSystem.
        engine.AddSystem(phx);                 // Physics comes after input
	    engine.AddSystem(rcm);                // Resource manager (loads resources on demand) MUST come before GameSystem or assets won't be found
        engine.AddSystem(audio);               //audio 
        engine.AddSystem(game);                // reads input -> moves/queues draws
        engine.AddSystem(gfx);                 // swaps buffers last

        engine.Initialize();
        engine.GameLoop();
	    SceneManager::DestroyInstance(); // Destroys the SceneManager instance at the end of the program.
        engine.DestroySystems();
        return 0;
    }
    catch (const GameException& e) {
        Logger logger;
        logger.logCrash(e.type, e.what());
    }
    catch (const std::exception& e) {
        Logger logger;
        logger.logCrash(DebugType::Unknown, e.what());
    }
    catch (...) {
        Logger logger;
        logger.logCrash(DebugType::Unknown, "Unhandled exception");
    }
}
