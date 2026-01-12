#include "TransitionController.h"
#include "ScriptRegistry.h"
#include "Components.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "OwalaTime.h"
#include <vector>     // Required for the list
#include <algorithm>  // Required for std::remove
#include "FileUtility.h"

namespace Framework {

    REGISTER_SCRIPT(TransitionController, "TransitionController")

    // Replace single instance with a list of all active controllers
    std::vector<TransitionController*> TransitionController::allControllers;

    const std::string CLOSE_ANIM_TEXTURE = "SB_Curtain_Close_Spritesheet_01.png";
    const std::string OPEN_ANIM_TEXTURE = "SB_Curtain_Open_Spritesheet_01.png";

    void TransitionController::OnCreate() {
        // Add this new entity to the global list
        allControllers.push_back(this);

		// Preload textures for open/close animations to make it smooth
		// Or else it will lag like hell the first time!!!
        auto* systems = GameManager::GetInstance().GetSystems().resourceManager_;
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
    }

    void TransitionController::OnDestroy() {
    }

    // Static Function to trigger CLOSE on ALL curtains
    void TransitionController::PlayAllClose(std::function<void()> onComplete) {
        if (allControllers.empty()) {
            // Safety: If no curtains exist, just run the callback immediately so the game continues
            if (onComplete) onComplete();
            return;
        }

        bool callbackAssigned = false;

        for (auto* controller : allControllers) {
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
            tex.textureKey = "";

            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);
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

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
            tex.textureKey = "";

            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(e);
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
        float dt = Framework::Time::UnscaledDeltaTime();

        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        if (!entityMgr.HasComponent(GetEntity(), TextureComponentType)) return;

        TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(GetEntity());
        
        if (state_ == TransitionState::CLOSING) {
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
        else if (state_ == TransitionState::OPENING) {
            if (animTimer_ == 0.0f) {
                tex.textureKey = OPEN_ANIM_TEXTURE;
            }

            animTimer_ += dt;
            float t = std::min(animTimer_ / animDuration_, 1.0f);

            if (t >= 1.0f) {
                state_ = TransitionState::HIDDEN;

                TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(GetEntity());
                tex.textureKey = "";

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
    }
}