#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PlayerSystem.h"
#include "PhysicsSystem.h"
#include "FileUtility.h"
#include "SceneManager.h"
#include "RotationSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "Logger.h"
#include "GameManager.h"

int main() {
    using namespace Framework;
    
    Logger::InitSymbols(); // need this to resolve the symbols in stack trace
    backward::SignalHandling sh; // global crash handler for segfaults, access violations, etc.

    try {
        CoreEngine engine;
	    FileIOSystem fileIO;
	    Vector2D windowConfig = fileIO.LoadWindowConfig();
        WindowsSystem *win = new WindowsSystem(engine, "Owala", (int)windowConfig.x, (int)windowConfig.y);
        InputSystem *input = new InputSystem(*win);
        EntityStorage *ecs = new EntityStorage();
        PhysicsSystem* phx = new PhysicsSystem(*ecs);
        SceneManager::GetInstance().SetEntityStorageRef(*ecs); // Creates SceneMgr Instance and sets the entity storage reference for the SceneManager
        RotationSystem* rotation = new RotationSystem(*ecs, *input, *phx);
        PlayerSystem* player = new PlayerSystem(*ecs, *input ,*phx);
        AudioSystem* audio = new AudioSystem(*ecs);
        Graphics *gfx = new Graphics(*win);
        ResourceManager* rcm = new ResourceManager(*gfx, *audio); //Create resource manager on the heap
        audio->SetResourceManager(*rcm); // Set resource managber for audio system
	    GameManager* gm = &GameManager::GetInstance(); // Create GameManager instance
	    gm->RegisterSystemsToUse(*rcm, *ecs, *gfx, *win, *input, *phx, *audio); // Register all core systems to the GameManager

        //    This is the core logic loop: Input -> Physics -> Gameplay -> Rendering.
        engine.AddSystem(win);                 // creates GLFW window + context
        engine.AddSystem(input);               // MUST come before GameSystem
        engine.AddSystem(rotation);
        engine.AddSystem(player);              // PlayerSystem processes input after InputSystem.
        engine.AddSystem(phx);                 // Physics comes after input
	    engine.AddSystem(rcm);                // Resource manager (loads resources on demand) MUST come before GameSystem or assets won't be found
        engine.AddSystem(audio);               //audio 
	    engine.AddSystem(gm);                  // GameManager for executing Game Logic
        engine.AddSystem(gfx);                 // swaps buffers last

        engine.Initialize();
        engine.GameLoop();
	    SceneManager::DestroyInstance(); // Destroys the SceneManager instance at the end of the program.
        engine.DestroySystems();
        return 0;
    }
    catch (const GameException& e) {
        Logger logger;
        logger.LogCrash(e.type, e.what(), e.stack_trace);
    }
    catch (const std::exception& e) {
        Logger logger;
        logger.LogCrash(DebugType::Unknown, e.what());
    }
    catch (...) {
        Logger logger;
        logger.LogCrash(DebugType::Unknown, "Unhandled exception");
    }
}
