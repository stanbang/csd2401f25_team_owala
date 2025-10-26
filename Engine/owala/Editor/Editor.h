/*********************************************************************//**
* \file   Editor.h
*
* \primary_author
*      Jason Lou (50%)
* \secondary_author
*      Lur Pei Hao Stanley (50%)
*
* \brief  Declares the Editor system for in-game level editing and
*         debugging using ImGUI.
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
#include "Graphics.h"
#include "ResourceManager.h"

// Imgui Dependencies
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "AssetBrowser.h"

namespace Framework {
	class GameManager;   // Forward Declaration
	class CoreEngine;    // Forward Declaration
	class CameraSystem;  // Forward Declaration
	struct SystemTime;   // Forward Declaration
}

namespace Editor {

	struct EditorSettings {
		bool showDebugInfo = true;
		bool showLevelEditor = true;
	};

	// Enum to represent the different types of objects we can create
	enum class PrefabType {
		None,
		Eraser,
		Platform,
		TrapPlatform,
		ScalingPlatform
	};

	// Structure to hold info about a draggable prefab for the palette
	struct DraggablePrefabInfo {
		std::string name;
		PrefabType type = PrefabType::None;
		Framework::Texture textureID = 0; // OpenGL Texture ID

		// tex coords (uses Dear ImGui vec struct)
		ImVec2 uv0{ 0.0f, 0.0f };
		ImVec2 uv1{ 1.0f, 1.0f };
	};

	class Editor : public Framework::ISystem {
	private:
		// --- System Pointers ---
		Framework::CoreEngine* engine_ = nullptr;             // Enforce Editor instantiated after engine
		Framework::GameManager* gameManager_ = nullptr;       // Must be created after GameManager
		Framework::CameraSystem* cameraSystem_ = nullptr;     // Pointer to CameraSystem for level editor
		Entity editorCamera_ = 0;                             // Dedicated camera for editor viewport
		Framework::Graphics* graphicsSystem_ = nullptr;       // Pointer to Graphics system
		Framework::ResourceManager* resourceManager_ = nullptr; // Pointer to ResourceManager for asset loading

		AssetBrowser assetBrowser;

		// --- Internal State ---
		bool initialized = false;
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

		// --- Tilemap Editor State ---
		bool isTileEditorActive_ = false;
		bool showGrid_ = false;
		float baseGridCellSize_ = 0.1f;
		float baseGridCellSizeX_ = 0.1f;
		float baseGridCellSizeY_ = 0.1f;

		// --- Drag and Drop State ---
		bool isDraggingPrefab_ = false;
		DraggablePrefabInfo dragDropPayload_;
		Vector2D dragDropPreviewPos_;

		// --- Palette Info Storage ---
		bool palettePrepared_ = false;
		std::vector<DraggablePrefabInfo> paletteItems_;

		// Entity Factory System Members
		Framework::PrefabSystem prefabSystem_;
		PrefabType selectedPrefabType_ = PrefabType::None;
		std::string selectedPrefabName_;

		// --- Entity Selection & Deletion ---
		Entity selectedEntity_ = 0;
		bool isEntitySelected_ = false;

		// --- Private Helper Functions ---
		void SanityCheck();
		Vector2D GetMouseWorldPosition();
		void HandleDragDropPlacement();
		void HandleTileErasing();
		void HandleSpawning();
		void HandleSelection();
		void HandleDeletion();

		// --- GUI Panels ---
		void ShowPerformanceProfile();
		void ShowLevelEditor();
		void ShowPlayerInspector();
		void ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent);
		void ShowTilemapEditor();
		void ShowAssetBrowser();

		// --- Grid Helper Functions ---
		void DrawEditorGrid();
		Vector2D SnapToGrid(const Vector2D& worldPos);
		float GetCurrentGridCellSize() const;
		Vector2D GetPrefabSize(const std::string& name, PrefabType type) const;

		// --- NEW Helpers ---
		void DrawDragDropPreview(); // Renders tile being dragged
		void PreparePaletteItems(); // Loads textures and prepares palette

	public:
		bool isEnabled = true;
		EditorSettings settings;

		// --- Constructors ---
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;
		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm);

		// --- Lifecycle Functions ---
		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override;
	};
}
