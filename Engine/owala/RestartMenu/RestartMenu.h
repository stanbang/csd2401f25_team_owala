#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "LevelResetSystem.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "GuiSystem.h"
#include "MessageBus.h"
#include "GameMessages.h"

namespace Framework {
    class RestartMenu : public ISystem {
    public:
        RestartMenu(EntityStorage& entityStorage, InputSystem& input,
            LevelResetSystem& levelReset, Graphics& graphics,
            ResourceManager& resourceManager, GuiSystem& guiSystem)
            : entityStorage_(&entityStorage), input_(&input),
            levelReset_(&levelReset), graphics_(&graphics),
            resourceManager_(&resourceManager), guiSystem_(&guiSystem) {}

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

    private:
        void RenderRestartMenuButtons();
        void HandleButtonClick(const ButtonClickedEvent& event);
        void RestartLevel();
        bool IsPlayerDead();
        void CleanupMenu();

        EntityStorage* entityStorage_ = nullptr;
        InputSystem* input_ = nullptr;
        LevelResetSystem* levelReset_ = nullptr;
        Graphics* graphics_ = nullptr;
        ResourceManager* resourceManager_ = nullptr;
        GuiSystem* guiSystem_ = nullptr;
        Font* menuFont_ = nullptr;

        // Button entities
        Entity restartButton= 0;
        Entity restartButtonText = 0;
        Entity quitButton = 0;
        Entity quitButtonText = 0;
        Entity menuBackground = 0;
        Entity deathTitle = 0;
        bool buttonsCreated = false;

        bool pendingRestart = false;
        bool pendingQuit = false;
    };
}