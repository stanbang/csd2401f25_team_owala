/*****************************************************************//**
 * \file  GuiSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implementation of the GuiSystem that handles GUI interactions.
 *			Uses mouse input to detect button hovering and clicks.
 *			Uses a subscription-publishing model to notify other systems of 
 *			button clicks.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "GuiSystem.h"
#include "GameManager.h"
#include "GameMessages.h" // For ButtonClickedEvent
#include "MessageBus.h" // For MessageBus

namespace Framework {

	void GuiSystem::Initialize() {
		//SetViewportSizeEngine(windowsSystem_->getWindowSize());
	}

	void GuiSystem::Update() {
		#ifdef BUILD_EDITOR
		if (engine_->IsEditorMode()) {
			return; // STOP here. Do not update buttons, do not render UI.
		}
		#endif

		GameManager& gm = GameManager::GetInstance();
		InputSystem& inputSystem_ = *gm.GetSystems().input_;
		EntityManager entityMgr(gm.GetECS());

		// Get mouse position in world coordinates
		double mouseX, mouseY;
		Vector2D viewPortSize;

		#ifdef BUILD_EDITOR
		// Convert to mouse position relative to the viewport's top-left corner
		// Get ABSOLUTE mouse position from ImGui
		ImVec2 absoluteMousePos = ImGui::GetIO().MousePos;
		mouseX = static_cast<double>(absoluteMousePos.x - viewportPos_.x);
		mouseY = static_cast<double>(absoluteMousePos.y - viewportPos_.y);
		double mouseX_ndc = static_cast<double>((mouseX / viewportSize_.x) * 2.0f - 1.0f);
		double mouseY_ndc = static_cast<double>(1.0f - (mouseY / viewportSize_.y) * 2.0f);
		#endif

		// Engine build uses full window size
		#ifdef BUILD_ENGINE
		WindowsSystem& windowsSystem_ = *gm.GetSystems().win_;
		inputSystem_.GetMousePos(&mouseX, &mouseY); // Get mouse position in window coordinates
		double mouseX_ndc = static_cast<double>((mouseX / windowsSystem_.getWidth()) * 2.0f - 1.0f);
		double mouseY_ndc = static_cast<double>(1.0f - (mouseY / windowsSystem_.getHeight()) * 2.0f);
		#endif

		bool mouseDown = inputSystem_.MouseDown(InputSystem::MouseButton::Left);

		// Loop through all active entities with Button, Position and AABB
		for (const Entity& button : entityMgr.GetAllEntitiesWithComponents(true, ButtonType, TransformType, TextureComponentType, LoadedTagType)){
			Button& buttonComponent = entityMgr.GetComponent<Button>(button);
			Transform& transform = entityMgr.GetComponent<Transform>(button);
			TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(button);

			// Calculate button bounds
			double minX = static_cast<double>(transform.position.x - tex.HalfSize.x);
			double maxX = static_cast<double>(transform.position.x + tex.HalfSize.x);
			double minY = static_cast<double>(transform.position.y - tex.HalfSize.y);
			double maxY = static_cast<double>(transform.position.y + tex.HalfSize.y);

			//Logger::Get().Info(DebugType::GUISystem) << "Mouse At: " << mouseX_ndc << ", " << mouseY_ndc << std::endl;
			//Logger::Get().Info(DebugType::GUISystem) << "Button At: " << minX << ", " << minY << " to " << maxX << ", " << maxY << std::endl;

			// Check if mouse is hovering over the button
			bool isHovering = (mouseX_ndc >= minX && mouseX_ndc <= maxX &&
				mouseY_ndc >= minY && mouseY_ndc <= maxY);


			// Update Button Hovering State
			buttonComponent.isHovered = isHovering;

			// Determine visual state
			Button::ButtonVisualState newVisualState = Button::ButtonVisualState::DEFAULT;
			
			// Update visual state based on hovering and mouse down
			if (isHovering && mouseDown) {
				newVisualState = Button::ButtonVisualState::PRESSED;
			}
			else if (isHovering) {
				newVisualState = Button::ButtonVisualState::HOVER;
			}

			// Switch textures when state changes
			if (newVisualState != buttonComponent.currentVisualState) {
				SwitchButtonTexture(button, buttonComponent, tex, newVisualState, entityMgr);
				buttonComponent.currentVisualState = newVisualState;
			}

			// Play hover sound on hover state change
			if (isHovering && !buttonComponent.wasHoveredLastFrame) {
				// Just started hovering
				if (!buttonComponent.hoverSoundKey.empty()) {
					PlayButtonSound(buttonComponent.hoverSoundKey, buttonComponent.hoverSoundVolume);
				}
			}
			buttonComponent.wasHoveredLastFrame = isHovering;

			// Check for mouse click release
			bool mouseClicked = inputSystem_.MouseReleased(InputSystem::MouseButton::Left);
			// If mouse clicked while hovering, trigger button action
			if (isHovering && mouseClicked) {
				Logger::Get().Info(DebugType::GUISystem)
					<< "Clicked: " << button << std::endl;
				
				// Play click sound
				if (!buttonComponent.clickSoundKey.empty()) {
					PlayButtonSound(buttonComponent.clickSoundKey, buttonComponent.clickSoundVolume);
				}
				
				// Reset button pressed state
				buttonComponent.isPressed = false;

				// Publish the event with the button's specific action
				// Pass the parameters from the component to the event
				MessageBus::Instance().Publish(ButtonClickedEvent{
						button,                     // Entity ID
						buttonComponent.action,     // The Enum (SWITCH_SCENE, PLAYSOUND, etc.)
						buttonComponent.targetString, // The payload (Scene Name or Sound Key)
						buttonComponent.targetValue   // The payload (Volume or other float)
					});
			}

			else if (isHovering && mouseDown) {
				// For visual feedback, set isPressed while mouse is down
				buttonComponent.isPressed = true;
			}

			else {
				buttonComponent.isPressed = false;
			}
		}
	}

	void GuiSystem::Shutdown() {

	}

	// Sets the viewport context for the editor (position and size)
	#ifdef BUILD_EDITOR
	void Framework::GuiSystem::SetViewportContext(ImVec2 position, ImVec2 size) {
		viewportPos_ = position;
		viewportSize_ = size;

		// Handle case where viewport is minimized
		if (viewportSize_.x == 0) viewportSize_.x = 1.0f;
		if (viewportSize_.y == 0) viewportSize_.y = 1.0f;
	}
	#endif

	// Play button sounds
	void GuiSystem::PlayButtonSound(const std::string& soundKey, float volume) {
		GameManager& gm = GameManager::GetInstance();
		AudioSystem* audioSystem_ = gm.GetSystems().audio_;
		ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;

		try {
			// Get the audio system through the resource manager or message bus
			// Assuming you have access to AudioSystem through a pointer
			if (audioSystem_) {
				FMOD::Sound& sound = resourceManager_->get<FMOD::Sound>(soundKey);
				audioSystem_->play(sound, volume, false);
			}
		}
		catch (const std::exception& e) {
			Logger::Get().Error(DebugType::GUISystem)
				<< "Failed to play button sound: " << soundKey
				<< " - " << e.what() << std::endl;
		}
	}

	// Switch button textures
    void GuiSystem::SwitchButtonTexture(Entity button, Button& buttonComp,
        TextureComponent& tex,
        Button::ButtonVisualState newState,
        EntityManager& entityMgr) {
        std::string newTextureKey;
        bool shouldAnimate = false;

        // Determine which texture to use
        switch (newState) {
        case Button::ButtonVisualState::PRESSED:
            if (!buttonComp.pressedTextureKey.empty()) {
                newTextureKey = buttonComp.pressedTextureKey;
                shouldAnimate = buttonComp.pressedIsAnimated;
            }
            else {
                newTextureKey = buttonComp.defaultTextureKey;
                shouldAnimate = buttonComp.defaultIsAnimated;
            }
            break;

        case Button::ButtonVisualState::HOVER:
            if (!buttonComp.hoverTextureKey.empty()) {
                newTextureKey = buttonComp.hoverTextureKey;
                shouldAnimate = buttonComp.hoverIsAnimated;
            }
            else {
                newTextureKey = buttonComp.defaultTextureKey;
                shouldAnimate = buttonComp.defaultIsAnimated;
            }
            break;

        case Button::ButtonVisualState::DEFAULT:
        default:
            newTextureKey = buttonComp.defaultTextureKey;
            shouldAnimate = buttonComp.defaultIsAnimated;
            break;
        }

        // Only switch if texture actually changed
        if (newTextureKey.empty() || newTextureKey == tex.textureKey) {
            return;
        }

        // Handle animation component
		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;
        if (shouldAnimate) {
            // Add AnimationComponent if it doesn't exist
            if (!entityMgr.HasComponent(button, AnimationComponentType)) {
                entityMgr.AddComponent<AnimationComponent>(button, AnimationComponent{});
            }

            AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(button);

            // Load animation info BEFORE setting texture key to prevent seeing a whole spritesheet for like a single frame
            if (resourceManager_.has<AnimationInfo>(newTextureKey)) {
                AnimationInfo& animInfo = resourceManager_.get<AnimationInfo>(newTextureKey);

                // Set animation properties from loaded data
                animComp.frameCount = animInfo.frameCount;
                animComp.frameDuration = animInfo.frameDuration;
                animComp.isLooping = animInfo.isLooping;
                animComp.startFrame = animInfo.startFrame;
                animComp.currentFrame = animInfo.startFrame;
                animComp.elapsedTime = 0.0f;
                animComp.isPlaying = true;

                // Calculate initial UV coordinates for first frame
                if (animInfo.frameCount > 0) {
                    // Check if multi-sheet animation
                    if (!animInfo.sheets.empty()) {
                        // Use first sheet
                        const SpriteSheetSegment& firstSheet = animInfo.sheets[0];
                        float frameUVWidth = 1.0f / (float)firstSheet.frameCount;
                        animComp.minUV = { 0.0f, 0.0f };
                        animComp.maxUV = { frameUVWidth, 1.0f };
                    }
                    else {
                        // Single sheet
                        float frameUVWidth = 1.0f / (float)animInfo.frameCount;
                        animComp.minUV = { 0.0f, 0.0f };
                        animComp.maxUV = { frameUVWidth, 1.0f };
                    }
                }
            }
            else {
                // Animation info not found, try to load it
                try {
                    resourceManager_.loadAnimationInfo(newTextureKey);

                    if (resourceManager_.has<AnimationInfo>(newTextureKey)) {
                        AnimationInfo& animInfo = resourceManager_.get<AnimationInfo>(newTextureKey);

                        animComp.frameCount = animInfo.frameCount;
                        animComp.frameDuration = animInfo.frameDuration;
                        animComp.isLooping = animInfo.isLooping;
                        animComp.startFrame = animInfo.startFrame;
                        animComp.currentFrame = animInfo.startFrame;
                        animComp.elapsedTime = 0.0f;
                        animComp.isPlaying = true;

                        // Calculate initial UVs
                        if (animInfo.frameCount > 0) {
                            if (!animInfo.sheets.empty()) {
                                const auto& firstSheet = animInfo.sheets[0];
                                float frameUVWidth = 1.0f / (float)firstSheet.frameCount;
                                animComp.minUV = { 0.0f, 0.0f };
                                animComp.maxUV = { frameUVWidth, 1.0f };
                            }
                            else {
                                float frameUVWidth = 1.0f / (float)animInfo.frameCount;
                                animComp.minUV = { 0.0f, 0.0f };
                                animComp.maxUV = { frameUVWidth, 1.0f };
                            }
                        }
                    }
                    else {
                        Logger::Get().Error(DebugType::GUISystem)
                            << "Failed to load animation info for: " << newTextureKey << std::endl;

                        // Fallback and show full texture
                        animComp.minUV = { 0.0f, 0.0f };
                        animComp.maxUV = { 1.0f, 1.0f };
                    }
                }
                catch (const std::exception& e) {
                    Logger::Get().Error(DebugType::GUISystem)
                        << "Exception loading animation info for " << newTextureKey
                        << ": " << e.what() << std::endl;

                    // Fallback
                    animComp.minUV = { 0.0f, 0.0f };
                    animComp.maxUV = { 1.0f, 1.0f };
                }
            }

        }
        else {
            // Remove AnimationComponent if texture is not animated
            if (entityMgr.HasComponent(button, AnimationComponentType)) {
                entityMgr.RemoveComponent(button, AnimationComponentType);
            }
        }

        // Update the texture key (after animation setup is complete)
        tex.textureKey = newTextureKey;
    }

}
