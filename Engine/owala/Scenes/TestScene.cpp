/*****************************************************************//**
 * \file	TestScene.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	20 September 2025
 *********************************************************************/
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Player.h"
#include "Platform.h"
#include "Background.h"
#include "Text.h"
#include "EntityStorage.h"
#include "RotationSystem.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"

#include "SpriteComponents.h"
#include "InputSystem.h"   // if you want toggles
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PhysicsSystem.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "TestScene.h"

 // ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {
    using namespace Framework;
    Entity playerEntity = 0;
    Entity groundEntity = 0;
    Entity backgroundEntity = 0;
}

namespace TestScene {
	using namespace Framework;

	void Initialize() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		WindowsSystem* win_ = gm.GetSystems().win_;

        // --- Load Assets ---
        // Ensure you have these files in an "Assets" folder next to your executable.
        //playerTex_ = gfx_->loadTextureFromFile("Assets/duck-rgba-256.tex");
        //groundTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png");
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs

        // --- Create Background Entity ---
        std::string backgroundID = "Background";
        backgroundEntity = createBackground(*ecs, { 0.0f, 0.0f }, backgroundID);
        sceneMgr.AddEntityToScene("Test Scene", backgroundEntity);

        // --- Create Ground Entity ---
        std::string platformTexID = "RonaldSprite_Platform";
        groundEntity = createPlatform(*ecs, { 0.0f, -0.5f }, { 1.0f, 0.2f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", groundEntity);

        // --- Create Player Entity ---
        FileIOSystem fileIO;
        PlayerStats stats = fileIO.LoadPlayerStats(); // Example stats from file loading

        std::string playerDuckID = "playerSprite_DuckyWucky";
        playerEntity = createPlayer(*ecs, stats, { 0.1f, 0.1f }, playerDuckID);
        sceneMgr.AddEntityToScene("Test Scene", playerEntity);
        sceneMgr.SetEntityAsPersistent(playerEntity);

        // --- Create Text Entity ---
        createText(*ecs, "abcdefg", "minecraft_font", Color(1,1,1,1), Vector2D(0,0), 1000.0f);

        // ---- AUDIO ----
        // Add AudioSystem BEFORE GameSystem in main.cpp so this is valid now.
        // BGM: loop + stream (e.g., WAV/OGG/MP3). Path must include extension.
        bool ok_bgm = audio->loadSound(
            "bgm",
            "../Assets/siren.ogg",   // <--- ensure this file exists
            /*loop*/ true,
            /*stream*/ true
        );
        if (!ok_bgm) {
            std::fprintf(stderr, "[Audio] Failed to load bgm\n");
        }
        audio->play("bgm", /*volume*/0.0f, /*paused*/false);

        // --- NEW: IMGUI INITIALIZATION ---
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking;     
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled, we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(win_->window(), true);
        ImGui_ImplOpenGL3_Init("#version 330");
	}

	void Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs_ = gm.GetSystems().ecs_;
		InputSystem* input_ = gm.GetSystems().input_;
		PhysicsSystem* physics_ = gm.GetSystems().physics_;
		Graphics* gfx_ = gm.GetSystems().gfx_;
        ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;
		AudioSystem* audio_ = gm.GetSystems().audio_;

        //  --- START THE IMGUI FRAME ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. --- HANDLE INPUTS ---
        // 'P' key toggles the master pause state.
        if (input_->KeyPressed(InputSystem::Key::P)) {
            physics_->SetPaused(!physics_->IsPaused());
        }

        // 'O' key toggles the step-by-step debug mode.
        if (input_->KeyPressed(InputSystem::Key::O)) {
            physics_->ToggleStepMode();
        }

        // 'I' key executes a single physics step if in step mode.
        if (input_->KeyPressed(InputSystem::Key::I)) {
            physics_->ExecuteNextStep();
        }

        if (input_->KeyPressed(InputSystem::Key::N)) {
            using namespace SceneManagement;
            SceneManager& sceneMgr = SceneManager::GetInstance();

            SceneAsset nextScene = sceneMgr.GetActiveScene().sceneIndex == 1 ? sceneMgr.GetSceneAt(2) : sceneMgr.GetSceneByName("Test Scene");
            sceneMgr.LoadScene(nextScene.name);
        }

        // 3. --- BUILD IMGUI INTERFACE ---
         // This is where you create all your ImGui windows and widgets.
        {
            // Create the main dockspace that will fill the entire game window.
            const ImGuiViewport* viewport = ImGui::GetMainViewport();

            // Example of a dockable "Level Editor" window.
            ImGui::Begin("Level Editor");

            if (ImGui::CollapsingHeader("Player Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Check if the player entity and its components exist.
                if (ecs_->position.count(playerEntity) && ecs_->velocity.count(playerEntity) && ecs_->playerState.count(playerEntity)) {

                    // Get references to the player's components.
                    auto& pos = ecs_->position.at(playerEntity).Pos;
                    auto& vel = ecs_->velocity.at(playerEntity).Vel;
                    auto& state = ecs_->playerState.at(playerEntity);

                    // Create ImGui widgets linked to the component data.
                    ImGui::DragFloat2("Position", &pos.x, 0.01f);
                    ImGui::InputFloat2("Velocity", &vel.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
                    ImGui::Checkbox("Is Grounded?", &state.isGrounded);

                    // A button to apply an action to the entity.
                    if (ImGui::Button("Reset Player Position")) {
                        pos = { 0.0f, 0.5f };
                        vel = { 0.0f, 0.0f };
                    }
                }
                else {
                    ImGui::Text("Player entity data not found!");
                }
            }

            if (ImGui::CollapsingHeader("Global Controls")) {
                static float masterVolume = 0.35f;
                if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
                    audio_->setMasterVolume(masterVolume);
                }
            }

            ImGui::End(); // End "Level Editor" window

            // You can also show the ImGui demo window to explore widgets.
            // ImGui::ShowDemoWindow(); 
        }

        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });


        //// Render background
        //if (ecs_->tex.count(backgroundEntity) && ecs_->position.count(backgroundEntity)) {
        //    const auto& pos = ecs_->position.at(backgroundEntity).Pos;

        //    //Use the textureID to get the actual texture from the resource manager
        //    std::string textureID = ecs_->tex[backgroundEntity].textureKey;
        //    Texture texture = resourceManager_->get<Texture>(textureID);
        //    gfx_->drawSprite(texture, pos, Vector2D(1, 1), 0.0f);
        //}

        //// Render the ground
        //if (ecs_->tex.count(groundEntity) && ecs_->position.count(groundEntity)) {
        //    const auto& pos = ecs_->position.at(groundEntity).Pos;
        //    const auto& halfSize = ecs_->colliders.at(groundEntity).HalfSize;
        //    float rotation = 0.0f;
        //    if (ecs_->rotationFlag.count(groundEntity)) {
        //        rotation = ecs_->rotationFlag.at(groundEntity).currentRotation * (M_PI / 180.0f);
        //    }
        //    Logger logger;
        //    // For now, we can use the console to see the state.
        //    if (physics_->IsInStepMode()) {
        //        logger.Info(DebugType::GameSystem, "STEP MODE ACTIVE (Press 'I' to advance)");
        //    }
        //    else if (physics_->IsPaused()) {
        //        logger.Info(DebugType::GameSystem, "PAUSED");
        //    }

        //    //Use the textureID to get the actual texture from the resource manager
        //    std::string textureID = ecs_->tex[groundEntity].textureKey;
        //    Texture texture = resourceManager_->get<Texture>(textureID);
        //    gfx_->drawSprite(texture, pos, halfSize, rotation);
        //}

        //// Render the player
        //if (ecs_->tex.count(playerEntity) && ecs_->position.count(playerEntity)) {
        //    const auto& pos = ecs_->position.at(playerEntity).Pos;
        //    const auto& halfSize = ecs_->colliders.at(playerEntity).HalfSize;


        //    std::string textureID = ecs_->tex[playerEntity].textureKey;
        //    //Use the textureID to get the actual texture from the resource manager
        //    Texture texture = resourceManager_->get<Texture>(textureID);
        //    gfx_->drawSprite(texture, pos, halfSize, 0.0f);
        //}

        // Render Font
        for (const auto &textComponentPair : ecs_->text) {
            Logger logger;
            const TextComponent& component = textComponentPair.second; 
            
            const auto& pos = ecs_->position.at(textComponentPair.first).Pos;
            std::string fontName = component.fontName;
            Font* font = resourceManager_->GetFont(fontName);
            if (font == nullptr) {
                logger.Info(DebugType::GraphicsSystem, "Font not found!!! " + fontName);
                continue;
            }

            gfx_->drawText(*font, component.textToDraw, pos, component.scale, component.color);
        }

        // 5. --- RENDER THE IMGUI UI ---
        // This draws the UI on top of the game world.
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        //   RenderColliders();
	}

	void Free() {
		// Empty by design
	}
}