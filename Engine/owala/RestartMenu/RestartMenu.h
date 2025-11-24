#pragma once
#include "EntityStorage.h"
#include "LevelResetSystem.h"
#include "ResourceManager.h"

namespace Framework {
    class RestartMenu {
    public:
        RestartMenu() = default;
        ~RestartMenu() = default;

        void Initialize(EntityStorage* ecs, LevelResetSystem* levelReset, ResourceManager* rcm);
        void HandleButtonClick(const ButtonClickedEvent& event);
        void Update();
        void CleanupMenu();
        void RestartLevel();
        bool IsPlayerDead();
        void Shutdown();

    private:
        EntityStorage* entityStorage_ = nullptr;
        LevelResetSystem* levelReset_ = nullptr;
        ResourceManager* resourceManager_ = nullptr;
        Font* menuFont_ = nullptr;

        bool buttonsCreated = false;
        bool pendingRestart = false;
        bool pendingQuit = false;

        Entity menuBackground = 0;
        Entity deathTitle = 0;
        Entity restartButton = 0;
        Entity restartButtonText = 0;
        Entity quitButton = 0;
        Entity quitButtonText = 0;

        void RenderRestartMenuButtons();
    };

} // namespace Framework
