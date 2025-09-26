#include <Windows.h>
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
#include "SpriteRenderSystem.h"
#include "GLFW/glfw3.h"

// Global pointer to WindowsSystem for console handler
Framework::WindowsSystem* g_win = nullptr;

// Console control handler to catch close events from the console window(aka pressing the 'X' button)
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_CLOSE_EVENT || signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        std::cout << "[ConsoleHandler] Console is closing. Triggering clean shutdown..." << std::endl;
        if (g_win) {
            glfwSetWindowShouldClose(g_win->window(), GLFW_TRUE); // triggers game loop exit
        }
        return TRUE;
    }
    return FALSE;
}

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	SetConsoleCtrlHandler(ConsoleHandler, TRUE); // Set the console control handler

    using namespace Framework;
    try {
    CoreEngine engine;
	FileIOSystem fileIO;
	Vector2D windowConfig = fileIO.LoadWindowConfig();
    WindowsSystem *win = new WindowsSystem(engine, "Owala", (int)windowConfig.x, (int)windowConfig.y);
	g_win = win; // Set global pointer for console handler
    InputSystem *input = new InputSystem(*win);
    std::unique_ptr<Framework::EntityStorage> ecs = std::make_unique<Framework::EntityStorage>();
    PhysicsSystem* phx = new PhysicsSystem(*ecs);
    SceneManager::GetInstance().SetEntityStorageRef(*ecs); // Creates SceneMgr Instance and sets the entity storage reference for the SceneManager
    RotationSystem* rotation = new RotationSystem(*ecs, *input, *phx);
    PlayerSystem* player = new PlayerSystem(*ecs, *input ,*phx);
    AudioSystem* audio = new AudioSystem(*ecs);
    Graphics *gfx = new Graphics(*win);
	ResourceManager* rcm = new ResourceManager(*gfx, *audio); //Create resource manager on the heap
	audio->SetResourceManager(*rcm); // Set resource manager for audio system
    GameManager* gm = &GameManager::GetInstance(); // Create GameManager instance
	gm->RegisterSystemsToUse(*rcm, *ecs, *gfx, *win, *input, *phx, *audio); // Register all core systems to the GameManager
    SpriteRenderSystem* renderer = new SpriteRenderSystem(*ecs, *gfx, *rcm, /*drawColliders*/false);

    //    This is the core logic loop: Input -> Physics -> Gameplay -> Rendering.
    engine.AddSystem(win);                 // creates GLFW window + context
    engine.AddSystem(input);               // MUST come before GameSystem
    engine.AddSystem(rotation);
    engine.AddSystem(player);              // PlayerSystem processes input after InputSystem.
    engine.AddSystem(phx);                 // Physics comes after input
    engine.AddSystem(audio);               //audio 
	engine.AddSystem(rcm);                // Resource manager MUST COME AFTER audio
	engine.AddSystem(gm);                  // GameManager for executing Game Logic
    engine.AddSystem(renderer); // <--- draw all sprites here
    engine.AddSystem(gfx);                 // swaps buffers last

    engine.Initialize();
    engine.GameLoop();
    engine.DestroySystems();
	SceneManager::DestroyInstance(); // Destroys the SceneManager instance at the end of the program.
    return 0;
    }
    catch (const GameException& e) {
        Logger logger;
        logger.LogCrash(e.type, e.what());
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
