/*********************************************************************//**
 * \file   main.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (16%)
 * \secondary_author
 *      Lur Pei Hao Stanley (16%)
 *      Sitara Anil (16%)
 *      Lou Jason (16%)
 *      Ng Jing Wen (16%)
 *      Yar Xuan (16%)
 *      Jordain Ng Ying Zhe (Documentation)
 *
 * \brief  Contains the main entry point for the application. All core systems
 *         are initialized and registered to the CoreEngine here.
 *
 * \details
 *      This file implements the application entry point and bootstraps the
 *      entire engine. It handles:
 *        - Memory leak detection setup
 *        - Console control handling (editor builds only)
 *        - Loading window configuration from file
 *        - Creating and registering all core engine systems
 *        - Editor integration (in editor builds)
 *        - Game application injection
 *        - Exception handling and crash logging
 *
 *      The system initialization order is carefully managed to ensure all
 *      dependencies are satisfied before systems start running.
 *
 * \note
 *      In editor builds, a console handler is installed to catch Ctrl+C
 *      and console close events for clean shutdown.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the
 *      prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include <Windows.h>
#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "GuiSystem.h"
#include "FileUtility.h"
#include "SceneManager.h"
#include "TransformationSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "Logger.h"
#include "GameManager.h"
#include "SpriteRenderSystem.h"
#include "TextRenderSystem.h"
#include "AnimationSystem.h"
#include "GLFW/glfw3.h"
#include "CameraSystem.h"
#include "PlatformBehaviorSystem.h"
#include "AISystem.h" 
#include "VFXSystem.h"
#include "Application.h"

#ifdef BUILD_EDITOR
 // Imgui Editor
#include "Editor.h"
#include "Gizmo.h"
#endif

// Only include Windows.h and console handler for Editor builds
#ifdef BUILD_EDITOR
#include <Windows.h>

// Global pointer to WindowsSystem for console handler
Framework::WindowsSystem* g_win = nullptr;

/**
 * @brief Console control handler to catch close events from the console window.
 *
 * Handles CTRL_CLOSE_EVENT, CTRL_C_EVENT, and CTRL_BREAK_EVENT by signaling
 * the GLFW window to close, initiating a clean shutdown.
 *
 * @param signal The control signal received
 * @return BOOL TRUE if handled, FALSE otherwise
 */
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_CLOSE_EVENT || signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        std::cout << "[ConsoleHandler] Console is closing. Triggering clean shutdown..." << std::endl;
        if (g_win) {
            glfwSetWindowShouldClose(g_win->window(), GLFW_TRUE);
        }
        return TRUE;
    }
    return FALSE;
}
#endif

#if defined(BUILD_EDITOR) && !defined(ACTIVE_GAME_INCLUDED)
// Stub implementation used when owala_Engine is built without any game source.
// Game builds (StageBound, NewGame, etc.) define their own CreateApplication.
// When ACTIVE_GAME_INCLUDED is defined (active_game.cmake set), the game's
// own CreateApplication is compiled in instead.
namespace Framework {
    /**
     * @brief Null application stub for engine-only builds.
     *
     * Used when building the engine without any game code (e.g., for testing
     * or as a standalone library). Provides empty implementations of all
     * required Application methods.
     */
    class NullApplication : public Application {
    public:
        void Initialize() override {}
        void Update()     override {}
        void Shutdown()   override {}
    };

    /**
     * @brief Creates a null application for engine-only builds.
     *
     * @param engine Reference to the core engine (unused)
     * @return Application* Pointer to a new NullApplication instance
     */
    Application* CreateApplication(CoreEngine&) { return new NullApplication(); }
}
#endif

/**
 * @brief Main entry point for the application.
 *
 * Bootstraps the entire engine:
 * 1. Sets up memory leak detection
 * 2. Installs console handler (editor builds)
 * 3. Creates CoreEngine and sets mode
 * 4. Loads window configuration
 * 5. Creates and registers all systems in correct order
 * 6. Initializes and runs the game loop
 * 7. Handles cleanup and exception logging
 *
 * @return int Return code (0 on success, non-zero on exception)
 */
int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // Only set up console handler for Editor builds
#ifdef BUILD_EDITOR
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif

    using namespace Framework;

    Logger::InitSymbols();
    backward::SignalHandling sh;

    CoreEngine engine;
    engine.SetMode(EngineMode::Play);

    try {
        // Load window configuration from file
        WindowConfig windowConfig = FileIOSystem::LoadWindowConfig();

        // Create window system first (depended on by many others)
        WindowsSystem* win = new WindowsSystem(engine, "Owala Engine",
            (int)windowConfig.windowSize.x,
            (int)windowConfig.windowSize.y,
            windowConfig.fullScreen);

#ifdef BUILD_EDITOR
        g_win = win; // Only needed for editor console handler
#endif

        // Get GameManager singleton
        GameManager* gm = &GameManager::GetInstance();
        gm->SetEngine(engine);

        // Create all core systems
        InputSystem* input = new InputSystem(*win);
        std::unique_ptr<Framework::EntityStorage> ecs = std::make_unique<Framework::EntityStorage>();
        PhysicsSystem* phx = new PhysicsSystem();
        TransformationSystem* transformation = new TransformationSystem();
        PlatformBehaviorSystem* platform = new PlatformBehaviorSystem();
        AudioSystem* audio = new AudioSystem();
        Graphics* gfx = new Graphics();
        AISystem* aiSystem = new AISystem();
        ResourceManager* rcm = new ResourceManager();
        AnimationSystem* animation = new AnimationSystem();
        GuiSystem* gui = new GuiSystem(engine);
        CameraSystem* camera = new CameraSystem();
        VFXSystem* vfx = new VFXSystem();
        SpriteRenderSystem* renderer = new SpriteRenderSystem(false);
        TextRenderSystem* textRenderer = new TextRenderSystem();

        // Register all systems with GameManager
        gm->RegisterSystemsToUse(*rcm, *ecs, *gfx, *win, *input, *phx, *audio,
            *renderer, *textRenderer, *camera, *aiSystem, *gui, *transformation, *animation);

        // Lazy create SceneManager
        SceneManager::GetInstance();

        // Editor Registration (only in editor builds)
#ifdef BUILD_EDITOR
        Editor::Editor* editor = new Editor::Editor(engine, *gm, *rcm);
        Editor::GizmoSystem* gizmo = new Editor::GizmoSystem(*editor);
        editor->SetGizmoSystem(gizmo);
        gm->RegisterEditor(*editor);
#endif

        // Add all systems to CoreEngine in appropriate order
        engine.AddSystem(win);
        engine.AddSystem(input);
        engine.AddSystem(transformation);
        engine.AddSystem(phx);
        engine.AddSystem(audio);
        engine.AddSystem(platform);
        engine.AddSystem(aiSystem);
        engine.AddSystem(vfx);
        engine.AddSystem(gm);
        engine.AddSystem(rcm);
        engine.AddSystem(camera);
        engine.AddSystem(animation);
        engine.AddSystem(gui);
#ifdef BUILD_EDITOR
        engine.AddSystem(editor);
#endif
        engine.AddSystem(gfx);

        // Injection of game application (game-specific code)
        Framework::Application* gameApp = Framework::CreateApplication(engine);
        engine.AddSystem(gameApp);

        // Run the engine
        engine.Initialize();
        engine.GameLoop();
        engine.DestroySystems();

        // Cleanup
        SceneManager::DestroyInstance();
#ifdef BUILD_EDITOR
        delete gizmo;
        gizmo = nullptr;
#endif
        return 0;
    }
    catch (const GameException& e) {
        // Handle engine-specific exceptions with stack trace
        Logger::Get().LogCrash(e.type, e.what(), e.stack_trace);
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }
    catch (const std::exception& e) {
        // Handle standard exceptions
        Logger::Get().LogCrash(DebugType::Unknown, e.what());
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }
    catch (...) {
        // Handle any other unknown exceptions
        Logger::Get().LogCrash(DebugType::Unknown, "Unhandled exception");
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }

    return 0;
}