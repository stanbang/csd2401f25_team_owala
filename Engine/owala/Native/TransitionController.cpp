/******************************************************************
 * \file   TransitionController.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the TransitionController script to manage curtain effects
 *          and states when transitioning between scenes.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "TransitionController.h"
#include "ScriptRegistry.h"
#include "Components.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "OwalaTime.h"
#include "FileUtility.h"

namespace Framework {

    REGISTER_SCRIPT(TransitionController, "TransitionController")

        // Replace single instance with a list of all active controllers
        std::vector<TransitionController*> TransitionController::allControllers;
    bool TransitionController::s_startNextSceneClosed = false;
    float TransitionController::s_lastOpenEventTime = -1.0f;

    const std::string CLOSE_ANIM_TEXTURE = "SB_Curtain_Close_Spritesheet_01.png";
    const std::string OPEN_ANIM_TEXTURE = "SB_Curtain_Open_Spritesheet_01.png";
    const std::string IDLE_ANIM_TEXTURE_R = "SB_Curtain_Idle_Spritesheet_R_01.png"; // Renamed for clarity
    const std::string IDLE_ANIM_TEXTURE_L = "SB_Curtain_Idle_Spritesheet_l_01.png";

    void TransitionController::OnCreate() {
        // Add this new entity to the global list
        allControllers.push_back(this);

        // Default to Right Curtain idle texture
        idleTextureKey_ = IDLE_ANIM_TEXTURE_R;
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        // Check position to determine if this is the Left Curtain.
        // Ensure 'TransformComponentType' matches your enum name for the Transform component.
        if (entityMgr.HasComponent(e, TransformType)) {
            Transform& trans = entityMgr.GetComponent<Transform>(e);

            // Assuming 0 is the center of the world/screen
            if (trans.position.x < 0.0f) {
                Logger::Get().Info(DebugType::GameManager) << "TransitionController: Detected Left Curtain based on position." << std::endl;
                idleTextureKey_ = IDLE_ANIM_TEXTURE_L;
            }
        }

        // Preload textures for open/close animations to make it smooth
        // Or else it will lag like hell the first time!!!
        ResourceManager* systems = GameManager::GetInstance().GetSystems().resourceManager_;
        if (!systems) return;
        ResourceManager& rcm = *systems;

        // Get the full paths for the keys
        const std::string mappingFilePath = "Data/textureToAnimationMapping.json";
        rapidjson::Document doc;

        if (!FileIOSystem::ReadFile(mappingFilePath, doc)) {
            // ReadFile already logs errors, so we just return if it fails.
            return;
        }

        if (doc.IsObject() && doc.HasMember("animations")) {
            const auto& animations = doc["animations"];

            for (const auto& entry : animations.GetArray()) {
                if (!entry.HasMember("animation_path") || !entry.HasMember("texture_keys")) continue;

                std::string path = entry["animation_path"].GetString();

                // Filter for "curtain" animations
                if (path.find("curtain") != std::string::npos || path.find("Curtain") != std::string::npos) {
                    const auto& keys = entry["texture_keys"];
                    for (const auto& keyVal : keys.GetArray()) {
                        std::string texKey = keyVal.GetString();

                        // Extract just the filename for asset path lookup
                        fs::path keyPath(texKey);
                        std::string filename = keyPath.filename().string();
                        std::string assetPath = rcm.GetAssetPath(filename);

                        if (!assetPath.empty() && !rcm.has<Texture>(filename)) {
                            rcm.loadTexture(filename, assetPath);
                        }
                    }
                }
            }
        }

        // Check flag immediately on creation
        CheckStartClosed();
    }

    void TransitionController::OnDestroy() {
        auto it = std::remove(allControllers.begin(), allControllers.end(), this);
        if (it != allControllers.end()) {
            allControllers.erase(it, allControllers.end());
        }
    }

    // Static Function to trigger CLOSE on ALL curtains
    void TransitionController::PlayAllClose(std::function<void()> onComplete) {
        if (allControllers.empty()) {
            // Safety: If no curtains exist, just run the callback immediately so the game continues
            if (onComplete) onComplete();
            return;
        }

        MessageBus::Instance().Publish(CurtainsCloseEvent());

        bool callbackAssigned = false;

        for (TransitionController* controller : allControllers) {
            // Only assign the callback to the FIRST controller found.
            // This prevents "LoadScene" from being called multiple times.
            if (!callbackAssigned) {
                controller->PlayClose(onComplete);
                callbackAssigned = true;
            }
            else {
                controller->PlayClose(nullptr); // Others play animation but don't trigger load
            }
        }
    }

    // Static Function to trigger OPEN on ALL curtains
    void TransitionController::PlayAllOpen() {
        for (auto* controller : allControllers) {
            controller->PlayOpen(nullptr);
        }
    }

    void TransitionController::PlayClose(std::function<void()> onComplete) {
        state_ = TransitionState::CLOSING;
        animTimer_ = 0.0f;
        onTransitionComplete_ = onComplete;

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
            tex.textureKey = CLOSE_ANIM_TEXTURE;

            if (entityMgr.HasComponent(e, TransformType)) {
                if (entityMgr.GetComponent<Transform>(e).position.x < 0.0f) {
                    tex.flipHorizontal = true;
                }
                else {
                    tex.flipHorizontal = false;
                }
            }

            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);

                ResourceManager* rsm = GameManager::GetInstance().GetSystems().resourceManager_;
                if (rsm) {
                    // Ensure data is loaded
                    if (!rsm->has<AnimationInfo>(CLOSE_ANIM_TEXTURE)) {
                        rsm->loadAnimationInfo(CLOSE_ANIM_TEXTURE);
                    }

                    if (rsm->has<AnimationInfo>(CLOSE_ANIM_TEXTURE)) {
                        AnimationInfo& info = rsm->get<AnimationInfo>(CLOSE_ANIM_TEXTURE);

                        // Update Component to match the texture (Fixes UVs/Frame Counts)
                        anim.frameCount = info.frameCount;
                        anim.frameDuration = info.frameDuration;

                        // Set the Logic Timer to match the Visual Duration
                        // If frameCount is 10 and duration is 0.2, total time is 2.0 seconds
                        animDuration_ = (float)info.frameCount * info.frameDuration;

                        // Safety check to prevent divide by zero later
                        if (animDuration_ <= 0.0f) animDuration_ = 1.0f;
                    }
                }

                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.isPlaying = true;
                anim.isLooping = false;
            }
        }
    }

    void TransitionController::PlayOpen(std::function<void()> onComplete) {
        state_ = TransitionState::OPENING;
        animTimer_ = 0.0f;
        onTransitionComplete_ = onComplete;

        float currentTime = Framework::Time::TotalRuntime();

        // Only publish if enough time has passed since the last one
        if (currentTime - s_lastOpenEventTime > 0.1f) {
            MessageBus::Instance().Publish(CurtainsOpenEvent());
            s_lastOpenEventTime = currentTime;
        }

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
            tex.textureKey = OPEN_ANIM_TEXTURE;

            if (entityMgr.HasComponent(e, TransformType)) {
                if (entityMgr.GetComponent<Transform>(e).position.x < 0.0f) {
                    tex.flipHorizontal = true; // Flip the Right Sprite to be Left
                }
                else {
                    tex.flipHorizontal = false;
                }
            }

            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);

                // Same logic for calculating duration as in PlayClose
                ResourceManager* rsm = GameManager::GetInstance().GetSystems().resourceManager_;
                if (rsm) {
                    if (!rsm->has<AnimationInfo>(OPEN_ANIM_TEXTURE)) {
                        rsm->loadAnimationInfo(OPEN_ANIM_TEXTURE);
                    }

                    if (rsm->has<AnimationInfo>(OPEN_ANIM_TEXTURE)) {
                        AnimationInfo& info = rsm->get<AnimationInfo>(OPEN_ANIM_TEXTURE);

                        anim.frameCount = info.frameCount;
                        anim.frameDuration = info.frameDuration;

                        // Sync logic timer to visual duration
                        animDuration_ = (float)info.frameCount * info.frameDuration;

                        if (animDuration_ <= 0.0f) animDuration_ = 1.0f;
                    }
                }

                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.isPlaying = true;
                anim.isLooping = false;
            }
        }
    }

    void TransitionController::SetClosed() {
        state_ = TransitionState::CLOSED;
    }

    void TransitionController::OnUpdate() {
        CheckStartClosed(); // Check flag each update in case it was set later

        float dt = Framework::Time::UnscaledDeltaTime();
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        if (!entityMgr.HasComponent(GetEntity(), TextureComponentType)) return;

        TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(GetEntity());
        // Get current scene and check if it's a level
        std::string currentScene = SceneManager::GetInstance().GetActiveScene().name;
        bool isLevel = GameManager::GetInstance().IsLevelScene(currentScene);

        // If we are in a non-level scene like the menu and the curtain is stuck CLOSED,
        // force it to hide immediately.
        if (!isLevel && state_ != TransitionState::HIDDEN
            && state_ != TransitionState::CLOSING
            && state_ != TransitionState::CLOSED) {
            ForceHide();
            return;
        }

        if (state_ == TransitionState::HIDDEN) {
            // If we are NOT in a level (e.g., MainMenu, LoadingScreen), ensure texture is empty.
            // This removes the "hanging curtains" that are preserved from the previous level.
            if (!isLevel && !tex.textureKey.empty()) {
                tex.textureKey = "";
            }
        }

        // If in IDLING state, curtains are visible at the sides with looping animation
        if (state_ == TransitionState::IDLING) {
            std::string targetIdleKey = IDLE_ANIM_TEXTURE_R;
            bool shouldFlip = false;

            // Determine if Left or Right Curtain based on position
            if (entityMgr.HasComponent(GetEntity(), TransformType)) {
                if (entityMgr.GetComponent<Transform>(GetEntity()).position.x < 0.0f) {
                    // Left Curtain
                    targetIdleKey = IDLE_ANIM_TEXTURE_L;
                    shouldFlip = false; // Dedicated sprite, DO NOT FLIP
                }
                else {
                    // Right Curtain
                    targetIdleKey = IDLE_ANIM_TEXTURE_R;
                    shouldFlip = false;
                }
            }

            // Ensure the idle animation is set and looping
            if (tex.textureKey != targetIdleKey || tex.flipHorizontal != shouldFlip) {
                tex.textureKey = targetIdleKey;
                tex.flipHorizontal = shouldFlip;

                if (entityMgr.HasComponent(GetEntity(), AnimationComponentType)) {
                    AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(GetEntity());

                    ResourceManager* rsm = GameManager::GetInstance().GetSystems().resourceManager_;
                    if (rsm) {
                        if (!rsm->has<AnimationInfo>(idleTextureKey_)) {
                            rsm->loadAnimationInfo(idleTextureKey_);
                        }

                        if (rsm->has<AnimationInfo>(idleTextureKey_)) {
                            AnimationInfo& info = rsm->get<AnimationInfo>(idleTextureKey_);
                            anim.frameCount = info.frameCount;
                            anim.frameDuration = info.frameDuration;
                        }
                    }

                    anim.currentFrame = 0;
                    anim.elapsedTime = 0.0f;
                    anim.isPlaying = true;
                    anim.isLooping = true; // Loop the idle animation
                }
            }
        }

        // If in WAITING state, increment timer, then transition to OPENING after delay
        if (state_ == TransitionState::WAITING) {
            waitTimer_ += dt;

            if (waitTimer_ >= WAIT_DURATION) {
                Logger::Get().Info(DebugType::GameManager) << "TransitionController: Wait complete. Opening..." << std::endl;
                PlayOpen(nullptr); // Transition to OPENING state
            }
        }

        // If in CLOSING state, play close animation
        else if (state_ == TransitionState::CLOSING) {
            if (animTimer_ == 0.0f) {
                tex.textureKey = CLOSE_ANIM_TEXTURE;
            }

            animTimer_ += dt;
            float t = std::min(animTimer_ / animDuration_, 1.0f);

            if (t >= 1.0f) {
                state_ = TransitionState::CLOSED;
                if (onTransitionComplete_) {
                    auto callback = onTransitionComplete_;
                    onTransitionComplete_ = nullptr;
                    callback();
                }
            }
        }
        // If in OPENING state, play open animation
        else if (state_ == TransitionState::OPENING) {
            if (animTimer_ == 0.0f) {
                tex.textureKey = OPEN_ANIM_TEXTURE;
            }

            animTimer_ += dt;
            float t = std::min(animTimer_ / animDuration_, 1.0f);

            if (t >= 1.0f) {
                // Transition to IDLING if in a level, otherwise HIDDEN
                if (isLevel) {
                    state_ = TransitionState::IDLING;
                }
                else {
                    state_ = TransitionState::HIDDEN;
                }

                if (onTransitionComplete_) {
                    auto callback = onTransitionComplete_;
                    onTransitionComplete_ = nullptr;
                    callback();
                }
            }
        }
    }

    TransitionController::~TransitionController() {
        // Remove self from list when object is deleted
        auto it = std::remove(allControllers.begin(), allControllers.end(), this);
        if (it != allControllers.end()) {
            allControllers.erase(it, allControllers.end());
        }
    }

    void TransitionController::Reset() {
        allControllers.clear();
        s_startNextSceneClosed = false;
    }

    void TransitionController::CheckStartClosed() {
        // If the flag is set, we apply the state to ALL controllers immediately
        // This ensures the first one to update doesn't steal the flag from the others.
        if (s_startNextSceneClosed) {

            Logger::Get().Info(DebugType::GameManager) << "TransitionController: Applying Start Closed to " << allControllers.size() << " curtains." << std::endl;

            for (TransitionController* controller : allControllers) {
                controller->StartWaitingState();
            }

            // Reset flag after applying to everyone
            s_startNextSceneClosed = false;
        }
    }

    void TransitionController::StartWaitingState() {
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);

            // Set Visuals to Closed (Frame 0 of Open Sheet)
            tex.textureKey = OPEN_ANIM_TEXTURE;

            if (entityMgr.HasComponent(e, TransformType)) {
                if (entityMgr.GetComponent<Transform>(e).position.x < 0.0f) {
                    tex.flipHorizontal = true;
                }
                else {
                    tex.flipHorizontal = false;
                }
            }

            // Setup Animation
            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);
                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.isPlaying = false; // Pause
                anim.isLooping = false;
            }

            // Set State
            state_ = TransitionState::WAITING;
            waitTimer_ = 0.0f;
        }
    }

    void TransitionController::ForceHideAll() {
        for (auto* controller : allControllers) {
            controller->ForceHide();
        }
    }

    void TransitionController::ForceHide() {
        state_ = TransitionState::HIDDEN;
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        if (entityMgr.HasComponent(GetEntity(), TextureComponentType)) {
            // Clear texture to make it invisible
            entityMgr.GetComponent<TextureComponent>(GetEntity()).textureKey = "";
        }
    }
}