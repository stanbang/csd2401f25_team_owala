/*****************************************************************//**
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
 * 
 * \brief  Contains the main entry point for the application. 
 * All core systems are initialized and registered to the CoreEngine here.
 * 
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include <Windows.h>
#include "Core.h"
#include "WindowsSystem.h"
#include "Graphics.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PlayerSystem.h"
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

// Console control handler to catch close events from the console window
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
        WindowConfig windowConfig = FileIOSystem::LoadWindowConfig();
        WindowsSystem* win = new WindowsSystem(engine, "Owala Engine",
            (int)windowConfig.windowSize.x,
            (int)windowConfig.windowSize.y,
            windowConfig.fullScreen);

#ifdef BUILD_EDITOR
        g_win = win; // Only needed for editor console handler
#endif

        GameManager* gm = &GameManager::GetInstance();
        gm->SetEngine(engine);
        InputSystem* input = new InputSystem(*win);
        std::unique_ptr<Framework::EntityStorage> ecs = std::make_unique<Framework::EntityStorage>();
        PhysicsSystem* phx = new PhysicsSystem();
        TransformationSystem* transformation = new TransformationSystem();
        PlatformBehaviorSystem* platform = new PlatformBehaviorSystem();
        PlayerSystem* player = new PlayerSystem();
        AudioSystem* audio = new AudioSystem();
        Graphics* gfx = new Graphics();
        AISystem* aiSystem = new AISystem();
        ResourceManager* rcm = new ResourceManager();
        //audio->SetResourceManager(*rcm);
        AnimationSystem* animation = new AnimationSystem();
        GuiSystem* gui = new GuiSystem(engine);
        CameraSystem* camera = new CameraSystem();
        VFXSystem* vfx = new VFXSystem();
        SpriteRenderSystem* renderer = new SpriteRenderSystem(false);
        TextRenderSystem* textRenderer = new TextRenderSystem();
        LevelResetSystem* levelResetter = new LevelResetSystem();

        gm->RegisterSystemsToUse(*rcm, *ecs, *gfx, *win, *input, *phx, *audio, *renderer, *textRenderer, *camera, *aiSystem, *gui, *levelResetter, *transformation, *animation);

        // Lazy create SceneManager.
        SceneManager::GetInstance();

        // Editor Registration
#ifdef BUILD_EDITOR
        Editor::Editor* editor = new Editor::Editor(engine, *gm, *rcm);
        Editor::GizmoSystem* gizmo = new Editor::GizmoSystem(*editor);
        editor->SetGizmoSystem(gizmo);
        gm->RegisterEditor(*editor);
#endif

        engine.AddSystem(win);
        engine.AddSystem(input);
        engine.AddSystem(transformation);
        engine.AddSystem(player);
        engine.AddSystem(phx);
        engine.AddSystem(audio);
        //engine.AddSystem(rcm);
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

        engine.Initialize();
        engine.GameLoop();
        engine.DestroySystems();
        SceneManager::DestroyInstance();
#ifdef BUILD_EDITOR
        delete gizmo;
        gizmo = nullptr;
#endif
        return 0;
    }
    catch (const GameException& e) {
        Logger::Get().LogCrash(e.type, e.what(), e.stack_trace);
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }
    catch (const std::exception& e) {
        Logger::Get().LogCrash(DebugType::Unknown, e.what());
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }
    catch (...) {
        Logger::Get().LogCrash(DebugType::Unknown, "Unhandled exception");
        engine.DestroySystems();
        SceneManager::DestroyInstance();
    }

    return 0;
}