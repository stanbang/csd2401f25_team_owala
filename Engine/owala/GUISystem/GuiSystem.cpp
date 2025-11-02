#include "GuiSystem.h"
#include "CameraSystem.h" // For the camera
#include "GameMessages.h" // For ButtonClickedEvent
#include "MessageBus.h" // For MessageBus

namespace Framework {


	void GuiSystem::Update() {

		// Get mouse position from InputSystem
		double mouseX, mouseY;
		inputSystem_->GetMousePos(&mouseX, &mouseY);

		bool mouseClicked = inputSystem_->MouseReleased(InputSystem::MouseButton::Left);
		bool mouseDown = inputSystem_->MouseDown(InputSystem::MouseButton::Left);

		// Loop through all active entities with Button, Position and AABB
		for (const Entity& button : entityMgr.GetAllEntitiesWithComponents(ButtonType, PositionType, AABBColliderType, ActiveTagType)){
			Button& buttonComponent = entityMgr.GetComponent<Button>(button);
			Position& position = entityMgr.GetComponent<Position>(button);
			AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(button);

			// Calculate button bounds
			float minX = position.Pos.x - collider.HalfSize.x;
			float maxX = position.Pos.x + collider.HalfSize.x;
			float minY = position.Pos.y - collider.HalfSize.y;
			float maxY = position.Pos.y + collider.HalfSize.y;

			// Check if mouse is hovering over the button
			bool isHovering = (mouseX >= minX && mouseX <= maxX && mouseY >= minY && mouseY <= maxY);

			// Update Button Hovering State
			buttonComponent.isHovered = isHovering;

			// If mouse clicked while hovering, trigger button action
			if (isHovering && mouseClicked) {
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
}
