/*****************************************************************//**
 * \file  GuiSystem.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Contains the declaration of the GuiSystem that handles GUI interactions.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "ResourceManager.h"
#include "EntityManager.h"
#include "InputSystem.h"
#include "OwalaTime.h"

#ifdef BUILD_EDITOR
// Imgui Editor
#include "Editor.h"
#endif

namespace Framework{
	class GuiSystem : public ISystem {
	public:
		// Constructor taking references to EntityStorage and ResourceManager
		GuiSystem(EntityStorage& entityStorage, ResourceManager& resourceManager, InputSystem& inputSystem)
			: entityStorage_(&entityStorage), resourceManager_(&resourceManager), inputSystem_(&inputSystem) {
		}
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

		// Sets the viewport context for the editor (position and size)
		#ifdef BUILD_EDITOR
		void SetViewportContext(ImVec2 position, ImVec2 size);
		#endif

		// Sets the viewport size for the engine (used outside of editor)
		void SetViewportSizeEngine(const Vector2D& size) {
			viewportSizeEngine_ = size;
		}

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		ResourceManager* resourceManager_ = nullptr;
		InputSystem* inputSystem_ = nullptr;

		Vector2D viewportSizeEngine_ = { 1.0f, 1.0f };

		#ifdef BUILD_EDITOR
		ImVec2 viewportPos_ = { 0.0f, 0.0f };
		ImVec2 viewportSize_ = { 1.0f, 1.0f };
		#endif


	};
}
