#include "GuiSystem.h"
#include "CameraSystem.h" // For the camera
#include "GameMessages.h" // For ButtonClickedEvent
#include "MessageBus.h" // For MessageBus

namespace Framework {

	void GuiSystem::Initialize() {

	}

	void GuiSystem::Update() {
		EntityManager entityMgr(*entityStorage_);

		// Get mouse position in world coordinates
		float mouseX, mouseY;
		Vector2D viewPortSize;

		#ifdef BUILD_EDITOR
		// Convert to mouse position relative to the viewport's top-left corner
		// Get ABSOLUTE mouse position from ImGui
		ImVec2 absoluteMousePos = ImGui::GetIO().MousePos;
		mouseX = absoluteMousePos.x - viewportPos_.x;
		mouseY = absoluteMousePos.y - viewportPos_.y;
		#endif

		#ifdef BUILD_ENGINE
		inputSystem_->GetMousePos(&mouseX, &mouseY);
		#endif

		float mouseX_ndc = (mouseX / viewportSize_.x) * 2.0f - 1.0f;
		float mouseY_ndc = 1.0f - (mouseY / viewportSize_.y) * 2.0f;

		bool mouseDown = inputSystem_->MouseDown(InputSystem::MouseButton::Left);

		// Loop through all active entities with Button, Position and AABB
		for (const Entity& button : entityMgr.GetAllEntitiesWithComponents(ButtonType, PositionType, TextureComponentType, ActiveTagType)){
			Button& buttonComponent = entityMgr.GetComponent<Button>(button);
			Position& position = entityMgr.GetComponent<Position>(button);
			TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(button);

			// Calculate button bounds
			float minX = position.Pos.x - tex.HalfSize.x;
			float maxX = position.Pos.x + tex.HalfSize.x;
			float minY = position.Pos.y - tex.HalfSize.y;
			float maxY = position.Pos.y + tex.HalfSize.y;

			//Logger::Get().Info(DebugType::GUISystem) << "Mouse At: " << mouseX_ndc << ", " << mouseY_ndc << std::endl;
			//Logger::Get().Info(DebugType::GUISystem) << "Button At: " << minX << ", " << minY << " to " << maxX << ", " << maxY << std::endl;

			// Check if mouse is hovering over the button
			bool isHovering = (mouseX_ndc >= minX && mouseX_ndc <= maxX &&
				mouseY_ndc >= minY && mouseY_ndc <= maxY);


			// Update Button Hovering State
			buttonComponent.isHovered = isHovering;

			bool mouseClicked = inputSystem_->MouseReleased(InputSystem::MouseButton::Left);

			// If mouse clicked while hovering, trigger button action
			if (isHovering && mouseClicked) {
				Logger::Get().Info(DebugType::GUISystem)
					<< "Clicked: " << button << std::endl;
				// Reset button pressed state
				buttonComponent.isPressed = false;

				// Publish ButtonClickedEvent
				MessageBus::Instance().Publish(ButtonClickedEvent{ button });
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

	void Framework::GuiSystem::SetViewportContext(ImVec2 position, ImVec2 size) {
		viewportPos_ = position;
		viewportSize_ = size;

		// Handle case where viewport is minimized
		if (viewportSize_.x == 0) viewportSize_.x = 1.0f;
		if (viewportSize_.y == 0) viewportSize_.y = 1.0f;
	}
}
