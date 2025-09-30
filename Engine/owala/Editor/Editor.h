/*****************************************************************//**
* \file   SceneComponents.h
*
* \primary_author
*      Jason Lou
* \secondary_author
*
*
* \brief	A subset of Components that are specific to scene management.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include "EntityStorage.h"
#include "PlatformPrefab.h" // prefab header for creation

namespace Framework {
	class GameManager; // Forward Declaration
	class CoreEngine; // Forward Declaration

	struct SystemTime; // Forward Declaration
}

namespace Editor {
	struct EditorSettings {
		bool showDebugInfo = true;
		bool showLevelEditor = true;
	};

	// enum to represent the different types of objects we can create
	enum class PrefabType {
		None,
		Platform,
		TrapPlatform,
		ScalingPlatform
	};

	class Editor : public Framework::ISystem {
	private:
		// --- System Pointers ---
		Framework::CoreEngine* engine_ = nullptr; // Enforce that the Editor must be instantiated after engine
		Framework::GameManager* gameManager_ = nullptr; // Enforce that the Editor must be instantiated after engine


		// --- Internal State ---
		bool initialized = false;
		//  Members for the system profiler display
		float profilerUpdateTimer_ = 0.0f;
		std::vector<Framework::SystemTime> profilerDisplayData_;
		float deltatime = 0.f;

		// Entity Inspector
		int selectedEntityIndex = 0;
		std::vector<std::string> componentNames;
		std::vector<const char*> componentItems;
		std::vector<Framework::ComponentType> componentTypes;
		int selectedComponentIndex = 0;
		std::vector<std::string> tagNames;
		std::vector<const char*> tagItems;
		std::vector<Framework::ComponentType> tagTypes;
		int selectedTagIndex = 0;

		// Entity factory System members
		Framework::PrefabSystem prefabSystem_; // Each editor instance has its own prefab system
		PrefabType selectedPrefabType_ = PrefabType::None;
		std::string selectedPrefabName_;

		// --- Entity Selection & Deletion ---
		Entity selectedEntity_ = 0; // Store entity ID
		bool isEntitySelected_ = false;  // Flag to track if an entity is selected

		// --- Private Helper Functions ---
		
		// Internal Checkers
		void SanityCheck();

		/**
		 * @brief Converts the current mouse position from screen coordinates to world coordinates.
		 * @return A Vector2D representing the mouse's position in the game world's normalized device coordinates (-1 to 1).
		 */
		Vector2D GetMouseWorldPosition(); // maybe can be in inputsystem.h in the future

		/**
		 * @brief Handles the logic for spawning a selected prefab at the mouse cursor position.
		 */
		void HandleSpawning();

		/**
		 * @brief Handles the logic for selecting an entity by clicking on it in the game world.
		 */
		void HandleSelection();
		void HandleDeletion();

		// Internal Panels
		void ShowPerformanceProfile();
		void ShowLevelEditor();
		void ShowPlayerInspector();
		void ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent);
	
	public:
		bool isEnabled = true;
		EditorSettings settings;

		/* Constructors */
		// Editors should not be cloneable or assignable. Only 1 should ever exist on the engine.
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager);

		/**
		 * @brief Initializes the Editor system.
		 * @details Loads prefabs from file and sets up the ImGui context and backends.
		 */
		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override;
	};
}