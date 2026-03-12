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

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        // Determine curtain side by entity name
        const std::string& entityName = entityMgr.GetProperties(e).name;
        isLeftCurtain_ = (entityName == "LeftCurtain");
		// Determine idle texture key based on curtain side (default to right if name doesn't match)
        idleTextureKey_ = isLeftCurtain_ ? IDLE_ANIM_TEXTURE_L : IDLE_ANIM_TEXTURE_R;

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

    /**
	 * During destruction, we need to remove this instance from the global list to prevent dangling pointers.
     */
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

		// Publish the event for any systems that need to react to curtains closing (e.g., to hide UI)
        MessageBus::Instance().Publish(CurtainsCloseEvent());
        MessageBus::Instance().Publish(PlaySoundEvent{ "Curtains_Close", AudioType::SFX, 1.0f });

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

	// Static Function to force all curtains to HIDDEN immediately (e.g., when starting a new scene)
    void TransitionController::PlayClose(std::function<void()> onComplete) {
        state_ = TransitionState::CLOSING;
        animTimer_ = 0.0f;
        onTransitionComplete_ = onComplete;

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);

			// Set the texture to the closing animation. The AnimationComponent will handle the frame updates.
            tex.textureKey = CLOSE_ANIM_TEXTURE;
            tex.flipHorizontal = isLeftCurtain_;

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

				// Reset animation to start from the beginning
                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.isPlaying = true;
                anim.isLooping = false;
            }
        }
    }

	// Non-static Function to trigger OPEN on this curtain
    void TransitionController::PlayOpen(std::function<void()> onComplete) {
        state_ = TransitionState::OPENING;
        animTimer_ = 0.0f;
        onTransitionComplete_ = onComplete;

		// Publish the event for any systems that need to react to curtains opening (e.g., to show UI)
        float currentTime = Framework::Time::TotalRuntime();
        // Only publish if enough time has passed since the last one
        if (currentTime - s_lastOpenEventTime > 0.1f) {
            MessageBus::Instance().Publish(CurtainsOpenEvent());
            MessageBus::Instance().Publish(PlaySoundEvent{ "Curtains_Open", AudioType::SFX, 1.0f });
            s_lastOpenEventTime = currentTime;
        }

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
			tex.textureKey = OPEN_ANIM_TEXTURE; // Set the texture to the opening animation. The AnimationComponent will handle the frame updates.
            tex.flipHorizontal = isLeftCurtain_;

            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);

                // Same logic for calculating duration as in PlayClose
                ResourceManager* rsm = GameManager::GetInstance().GetSystems().resourceManager_;
                if (rsm) {
                    if (!rsm->has<AnimationInfo>(OPEN_ANIM_TEXTURE)) {
                        rsm->loadAnimationInfo(OPEN_ANIM_TEXTURE);
                    }

					// If the animation info exists, use it to set frame counts and durations. This ensures that if the anim data changes, we don't have to hardcode it here.
                    if (rsm->has<AnimationInfo>(OPEN_ANIM_TEXTURE)) {
                        AnimationInfo& info = rsm->get<AnimationInfo>(OPEN_ANIM_TEXTURE);

                        anim.frameCount = info.frameCount;
                        anim.frameDuration = info.frameDuration;

                        // Sync logic timer to visual duration
                        animDuration_ = (float)info.frameCount * info.frameDuration;

                        if (animDuration_ <= 0.0f) animDuration_ = 1.0f;
                    }
                }

				// Reset animation to start from the beginning
                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.isPlaying = true;
                anim.isLooping = false;
            }
        }
    }

	// Static Function to force this curtain to CLOSED immediately without animation (e.g., when starting directly in a loading screen)
    void TransitionController::SetClosed() {
        state_ = TransitionState::CLOSED;
    }

    void TransitionController::OnUpdate() {
        ResourceManager* rsm = GameManager::GetInstance().GetSystems().resourceManager_;
        CheckStartClosed(); // Check flag each update in case it was set later

        float dt = Framework::Time::UnscaledDeltaTime();
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        if (!entityMgr.HasComponent(GetEntity(), TextureComponentType)) return;

        TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(GetEntity());
        // Check if any currently loaded scene (excluding DDOL) is a gameplay level.
        // This ensures curtains remain visible when overlay menus (PauseScene, OptionsMenu)
        // are additively loaded on top of a level.
        const std::vector<SceneGUID>& loadedScenes = SceneManager::GetInstance().GetLoadedScenes();
        constexpr SceneGUID ddolId = SceneManager::GetDDOLSceneID();
        bool isLevel = false;
        for (SceneGUID guid : loadedScenes) {
            if (guid == ddolId) continue;
            const SceneAsset& scene = SceneManager::GetInstance().GetSceneAt(guid);
            if (scene.IsValid() && GameManager::GetInstance().IsLevelScene(scene.name)) {
                isLevel = true;
                break;
            }
        }

        // If we are in a non-level scene like the menu and the curtain is stuck CLOSED,
        // force it to hide immediately.
        if (!isLevel && state_ != TransitionState::HIDDEN
            && state_ != TransitionState::CLOSING
            && state_ != TransitionState::CLOSED) {
            ForceHide();
            return;
        }

		// Evaluate WAITING state before anything else - we want to delay the OPENING animation even if the scene isn't a level
        if (state_ == TransitionState::WAITING) {
            waitTimer_ += dt;
            if (waitTimer_ >= WAIT_DURATION) {
                PlayOpen(nullptr);
            }
        }
		// If in closing animation, update timer and transition to CLOSED when done
        else if (state_ == TransitionState::CLOSING) {
            if (animTimer_ == 0.0f) tex.textureKey = CLOSE_ANIM_TEXTURE;

            // Enforce isLooping = false to prevent AnimationSystem overrides
            if (entityMgr.HasComponent(GetEntity(), AnimationComponentType)) {
                entityMgr.GetComponent<AnimationComponent>(GetEntity()).isLooping = false;
            }

            animTimer_ += dt;
            float t = std::min(animTimer_ / animDuration_, 1.0f);

			// When closing animation finishes, we switch to CLOSED state and invoke the callback to trigger the scene load. 
            // We wait until the animation finishes to switch states so that the curtains remain visible during the transition, 
            // instead of snapping away immediately when the scene loads.
            if (t >= 1.0f) {
                state_ = TransitionState::CLOSED;
                if (onTransitionComplete_) {
                    auto callback = onTransitionComplete_;
                    onTransitionComplete_ = nullptr;
                    callback();
                }
            }
        }
		// If in opening animation, update timer and transition to IDLING or HIDDEN when done,  de
        else if (state_ == TransitionState::OPENING) {
            if (animTimer_ == 0.0f) tex.textureKey = OPEN_ANIM_TEXTURE;

            // Enforce isLooping = false to prevent AnimationSystem overrides
            if (entityMgr.HasComponent(GetEntity(), AnimationComponentType)) {
                entityMgr.GetComponent<AnimationComponent>(GetEntity()).isLooping = false;
            }

            animTimer_ += dt;
            float t = std::min(animTimer_ / animDuration_, 1.0f);

            if (t >= 1.0f) {
                // State changes here will be caught immediately by the passive checks below
                if (isLevel) {
                    state_ = TransitionState::IDLING;
                }
                else {
                    state_ = TransitionState::HIDDEN;
                }

				// Invoke callback after animation finishes and state is updated, so that any logic triggered by the callback can check the correct state of the curtains.
                if (onTransitionComplete_) {
                    auto callback = onTransitionComplete_;
                    onTransitionComplete_ = nullptr;
                    callback();
                }
            }
        }

		// Evaluate passive states after animations - this handles the case where we start in a non-level scene and need to hide immediately, or when we transition to a non-level scene and need to hide immediately, even if we didn't start closed.
        if (state_ == TransitionState::HIDDEN) {
            // If we are NOT in a level (e.g., MainMenu, LoadingScreen), ensure texture is empty.
            // This removes the "hanging curtains" that are preserved from the previous level.
            if (!isLevel && !tex.textureKey.empty()) {
                tex.textureKey = "";
            }
        }

		// If the curtains are supposed to be hidden but the texture is still set 
        // (e.g., we transitioned to a non-level scene but the curtain texture is still there from the previous level), 
        // clear the texture to hide them. This is important for ensuring that if we start in a non-level scene, we don't show the curtains until we want to.
        if (state_ == TransitionState::HIDDEN) {
            if (!isLevel && !tex.textureKey.empty()) {
                tex.textureKey = "";
            }
        }

		// When idling, we want to ensure the correct idle texture is playing. This is important for two reasons:
        else if (state_ == TransitionState::IDLING) {
            std::string targetIdleKey = idleTextureKey_;

			// Check if the target idle animation is already playing to avoid resetting it and 
            // causing a visual hiccup. We check both the main texture key and any associated spritesheet 
            // segments in the AnimationInfo, since some animations use separate textures for each frame.
            bool isAlreadyPlayingIdle = false;
            if (rsm && rsm->has<AnimationInfo>(targetIdleKey)) {
                AnimationInfo& info = rsm->get<AnimationInfo>(targetIdleKey);

				// First check if the main texture key matches the target idle key
                if (tex.textureKey == targetIdleKey) {
                    isAlreadyPlayingIdle = true;
                }

				// If not, check if any of the spritesheet segments match the target idle key
                else {
                    for (const auto& sheet : info.sheets) {
                        if (tex.textureKey == sheet.textureKey) {
                            isAlreadyPlayingIdle = true;
                            break;
                        }
                    }
                }
            }
            else if (tex.textureKey == targetIdleKey) {
                isAlreadyPlayingIdle = true;
            }

			// If we're not already playing the correct idle animation, switch to it. This ensures that if we just finished the opening animation, 
            // we transition smoothly into the idle animation without any visual hiccups or flashes.
            if (!isAlreadyPlayingIdle) {
                tex.textureKey = targetIdleKey;
                tex.flipHorizontal = false;

                if (entityMgr.HasComponent(GetEntity(), AnimationComponentType)) {
                    AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(GetEntity());

                    if (rsm) {
                        if (!rsm->has<AnimationInfo>(targetIdleKey)) {
                            rsm->loadAnimationInfo(targetIdleKey);
                        }
                        if (rsm->has<AnimationInfo>(targetIdleKey)) {
                            AnimationInfo& info = rsm->get<AnimationInfo>(targetIdleKey);
                            anim.frameCount = info.frameCount;
                            anim.frameDuration = info.frameDuration;

                            // AnimationSystem already ran this frame with the open animation's UVs.
                            // Immediately write correct frame-0 UVs for the idle sheet so the
                            // renderer never sees the idle texture bound with open-animation UVs.
                            // This eliminates the one-frame ghost/flash at the transition point.
                            constexpr float UV_PADDING = 0.003f;
                            if (!info.sheets.empty()) {
                                // Multi-sheet: resolve which sheet owns frame 0 and set its key + UVs
                                tex.textureKey = info.sheets[0].textureKey;
                                const int sheetFrameCount = info.sheets[0].frameCount;
                                if (sheetFrameCount > 0) {
                                    const float frameUVWidth = 1.0f / static_cast<float>(sheetFrameCount);
                                    anim.minUV.x = UV_PADDING;
                                    anim.maxUV.x = frameUVWidth - UV_PADDING;
                                }
                            }
                            else if (info.frameCount > 0) {
                                // Single-sheet: frame 0 UV
                                const float frameUVWidth = 1.0f / static_cast<float>(info.frameCount);
                                anim.minUV.x = UV_PADDING;
                                anim.maxUV.x = frameUVWidth - UV_PADDING;
                            }
                            anim.minUV.y = 0.0f;
                            anim.maxUV.y = 1.0f;
                        }
                    }

                    anim.currentFrame = 0;
                    anim.elapsedTime = 0.0f;
                    anim.isPlaying = true;
                    anim.isLooping = true;
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

	// Static function to reset all controllers - useful when starting a new game or returning to main menu to clear any lingering states
    void TransitionController::Reset() {
        allControllers.clear();
        s_startNextSceneClosed = false;
    }

	// Static function to check if we need to start with curtains closed, and if so, apply it to all controllers. This is called on each controller's OnCreate, but the flag is only applied once to all controllers to ensure consistency.
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

	// This function is called on each controller when the Start Closed flag is set. It sets the curtain to the 
    // OPEN animation but immediately pauses it at the first frame, effectively showing the closed curtain without 
    // playing any animation. After a short wait, it will then play the opening animation as normal. This allows us 
    // to start in a scene with the curtains closed and then have them open smoothly after a brief pause.
    void TransitionController::StartWaitingState() {
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
            tex.textureKey = OPEN_ANIM_TEXTURE;
            tex.flipHorizontal = isLeftCurtain_;

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

	// This function is used to immediately hide the curtains without 
    // playing any animations. It sets the state to HIDDEN and clears the texture key, 
    // effectively making the curtains invisible. This is useful for non-level scenes 
    // where we don't want the curtains to be visible at all, or if we need to force 
    // them hidden for any reason.
    void TransitionController::ForceHideAll() {
        for (auto* controller : allControllers) {
            controller->ForceHide();
        }
    }

	// This function forces this curtain to be hidden immediately by setting the state 
    // to HIDDEN and clearing the texture key. This is used in non-level scenes to ensure 
    // that the curtains are not visible when they shouldn't be, such as in the main menu or 
    // loading screens.
    void TransitionController::ForceHide() {
        state_ = TransitionState::HIDDEN;
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        if (entityMgr.HasComponent(GetEntity(), TextureComponentType)) {
            // Clear texture to make it invisible
            entityMgr.GetComponent<TextureComponent>(GetEntity()).textureKey = "";
        }
    }

}