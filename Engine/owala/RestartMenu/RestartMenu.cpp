#include "RestartMenu.h"
#include "EntityManager.h"
#include "SceneManager.h" 
#include <iostream>

namespace Framework {
    void RestartMenu::Initialize() {
        // Load font for menu text
        try {
            menuFont_ = &resourceManager_->get<Font>("Amethysta-Regular.ttf");
        }
        catch (...) {
            std::cerr << "Warning: Menu font not loaded." << std::endl;
            menuFont_ = nullptr;
        }
        // Subscribe to button click events
        MessageBus::Instance().Subscribe<ButtonClickedEvent>(
            std::bind(&RestartMenu::HandleButtonClick, this, std::placeholders::_1)
        );
    }

    void RestartMenu::HandleButtonClick(const ButtonClickedEvent& event) {
        if (!IsPlayerDead()) return;

        EntityManager entityMgr(*entityStorage_);
        std::string entityName = entityMgr.GetProperties(event.button).name;

        std::cout << "Button clicked: " << entityName << std::endl;

        if (entityName == "RestartButton") {
            std::cout << "Restart pressed..." << std::endl;
            pendingRestart = true;
        }
        else if (entityName == "QuitButton") {
            std::cout << "Quit to menu pressed" << std::endl;
            pendingQuit = true;
        }
    }

    void RestartMenu::Update() {
        if (pendingRestart) {
            RestartLevel();
            pendingRestart = false;
            return;
        }

        if (pendingQuit) {
            // TODO: Add quit to menu logic here
            std::cout << "Executing quit to menu..." << std::endl;
            pendingQuit = false;
            return;
        }

        if (IsPlayerDead()) {
            if (!buttonsCreated) {
                RenderRestartMenuButtons();
                buttonsCreated = true;
            }
        }
        else {
            if (buttonsCreated) {
                CleanupMenu();
            }
        }
    }

    void RestartMenu::RenderRestartMenuButtons() {
        EntityManager entityMgr(*entityStorage_);
        Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

        menuBackground = entityMgr.CreateEntity();
        entityMgr.GetProperties(menuBackground).name = "RestartMenuBackground";
        entityMgr.AddComponent(menuBackground, Position{ {0.0f, 0.0f} });
        entityMgr.AddComponent(menuBackground, TextureComponent{ /*Background.png*/"", {1.0f, 1.0f} });
        entityMgr.AddComponent(menuBackground, GUITag{});
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), menuBackground);

        deathTitle = entityMgr.CreateEntity();
        entityMgr.GetProperties(deathTitle).name = "DeathTitle";
        entityMgr.AddComponent(deathTitle, Position{ {-0.15f, 0.4f} });
        entityMgr.AddComponent(deathTitle, GUITag{});
        entityMgr.AddComponent(deathTitle, TextComponent{ "YOU DIED", "Amethysta-Regular.ttf", {1.0f, 0.2f, 0.2f, 1.0f}, 1.2f });
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), deathTitle);

        restartButton = entityMgr.CreateEntity();
        entityMgr.GetProperties(restartButton).name = "RestartButton";
        entityMgr.AddComponent(restartButton, Position{ {0.0f, 0.1f} });
        entityMgr.AddComponent(restartButton, TextureComponent{ "Button.png", {0.2f, 0.2f} });
        entityMgr.AddComponent(restartButton, Button{});
        entityMgr.AddComponent(restartButton, GUITag{});
        entityMgr.AddComponent(restartButton, ActiveTag{});
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), restartButton);
        restartButtonText = entityMgr.CreateEntity();
        entityMgr.GetProperties(restartButtonText).name = "RestartButtonText";
        entityMgr.AddComponent(restartButtonText, Position{ {-0.13f, 0.1f} });
        entityMgr.AddComponent(restartButtonText, GUITag{});
        entityMgr.AddComponent(restartButtonText, TextComponent{ "Restart Level", "Amethysta-Regular.ttf", {1.0f, 1.0f, 1.0f, 1.0f}, 0.8f });
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), restartButtonText);

        quitButton = entityMgr.CreateEntity();
        entityMgr.GetProperties(quitButton).name = "QuitButton";
        entityMgr.AddComponent(quitButton, Position{ {0.0f, -0.20f} });
        entityMgr.AddComponent(quitButton, TextureComponent{ "Button.png", {0.2f, 0.2f} });
        entityMgr.AddComponent(quitButton, Button{});
        entityMgr.AddComponent(quitButton, GUITag{});
        entityMgr.AddComponent(quitButton, ActiveTag{});
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), quitButton);

        quitButtonText = entityMgr.CreateEntity();
        entityMgr.GetProperties(quitButtonText).name = "QuitButtonText";
        entityMgr.AddComponent(quitButtonText, Position{ {-0.13f, -0.20f} });
        entityMgr.AddComponent(quitButtonText, GUITag{});
        entityMgr.AddComponent(quitButtonText, TextComponent{ "Quit to Menu", "Amethysta-Regular.ttf", {1.0f, 1.0f, 1.0f, 1.0f}, 0.8f });
        sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), quitButtonText);

    }

    void RestartMenu::CleanupMenu() {
        EntityManager entityMgr(*entityStorage_);

        if (restartButton != 0) {
            entityMgr.DestroyEntity(restartButton);
            restartButton = 0;
        }
        if (restartButtonText!= 0) {
            entityMgr.DestroyEntity(restartButtonText);
            restartButtonText = 0;
        }
        if (quitButton != 0) {
            entityMgr.DestroyEntity(quitButton);
            quitButton = 0;
        }
        if (quitButtonText != 0) {
            entityMgr.DestroyEntity(quitButtonText);
            quitButtonText = 0;
        }
        if (menuBackground != 0) {
            entityMgr.DestroyEntity(menuBackground);
            menuBackground = 0;
        }
        if (deathTitle != 0) {
            entityMgr.DestroyEntity(deathTitle);
            deathTitle = 0;
        }

        buttonsCreated = false;
    }

    void RestartMenu::RestartLevel() {
        levelReset_->RunReset();
        CleanupMenu();
    }

    void RestartMenu::Shutdown() {
        if (buttonsCreated) {
            CleanupMenu();
        }
    }

    bool RestartMenu::IsPlayerDead() {
        EntityManager entityMgr(*entityStorage_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            if (entityMgr.HasComponent(entity, PlayerStateType)) {
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                if (state.isDead) {
                    if (entityMgr.HasComponent(entity, AnimationComponentType)) {
                        AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);
                        if (anim.currentState == AnimationState::DEATH && !anim.isPlaying) {
                            return true;
                        }
                    }
                    else {
                        return true;
                    }
                }
            }
        }
        return false;
    }
}