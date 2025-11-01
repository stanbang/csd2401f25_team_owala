/*********************************************************************//**
* \file   Editor.h
*
* \primary_author
*      Jason Lou (50%)
* \secondary_author
*	   Lur Pei Hao Stanley (50%)
*
*
* \brief	Declares the Editor system for in-game level editing and 
			debugging using ImGUI.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include <filesystem>
#include "EntityStorage.h"
#include "PrefabSystem.h" // prefab header for creation
#include "Graphics.h"
#include "ResourceManager.h"
#include "SceneManager.h"

// Imgui Dependencies
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "AssetBrowser.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <optional>

namespace Framework {
	class GameManager; // Forward Declaration
	class CoreEngine; // Forward Declaration
	class CameraSystem;
	struct SystemTime; // Forward Declaration
	
}

namespace Editor {

	struct EditorSettings {
		bool showDebugInfo = true;
		bool showLevelEditor = true;
		bool showHierarchy = true;
		bool showInspector = true;
		bool showGameViewport = true;
		bool showAssetBrowser = true;
		bool showToolbar = true;
	};

	// enum to represent the different types of objects we can create
	enum class PrefabType {
		None,
		Eraser,
		Platform,
		TrapPlatform,
		ScalingPlatform
	};

	// NEW: Structure to hold info about a draggable prefab for the palette
	struct DraggablePrefabInfo {
		std::string name;
		PrefabType type = PrefabType::None;
		Framework::Texture textureID = 0; // OpenGL Texture ID
		// tex coords(uses dearimgui vec struct)
		ImVec2 uv0{ 0.0f, 0.0f }; 
		ImVec2 uv1{ 1.0f, 1.0f }; 
	};

	class Editor : public Framework::ISystem {
	private:
		// --- System Pointers ---
		Framework::CoreEngine* engine_ = nullptr; // Enforce that the Editor must be instantiated after engine
		Framework::GameManager* gameManager_ = nullptr; // Enforce that the Editor must be instantiated after GameManager
		Framework::CameraSystem* cameraSystem_ = nullptr; // Pointer to camera systems for level editor
		Framework::Graphics* graphicsSystem_ = nullptr; //  Pointer to Graphics system
		Framework::ResourceManager* resourceManager_ = nullptr; // Pointer to ResourceManager for asset loading
		Entity editorCamera_ = 0;
		float editorZoom_ = 1.0f;
		Vector2D editorCamPos_ = { 0.0f, 0.0f };
		bool isEditorFocused_ = true; // toggled when ImGui viewport or editor window is active

		AssetBrowser assetBrowser;

		// --- Internal State ---
		bool initialized = false;
		bool firstUpdate = true;
		bool isGamePlaying = false;
		bool isGamePaused = false;
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
		std::vector<const char*> fontKeys;

		// --- Tilemap Editor State ---
		bool isTileEditorActive_ = false; //  Flag to control tile editor mode
		bool showGrid_ = false;
		float baseGridCellSize_ = 0.1f;   // NEW: World-space size of a cell at zoom 1.0
		

		//Drag and Drop State
		bool isDraggingPrefab_ = false;
		DraggablePrefabInfo dragDropPayload_; // Holds info about the prefab being dragged
		Vector2D dragDropPreviewPos_; // World position for preview rendering

		//Palette Info storage
		bool palettePrepared_ = false; // flag for storing palette items
		std::vector<DraggablePrefabInfo> paletteItems_;

		// Game viewport tracking for proper mouse coordinate transformation
		ImVec2 gameViewportPos_ = { 0, 0 };    // Top-left position of game viewport in screen space
		ImVec2 gameViewportSize_ = { 0, 0 };   // Size of game viewport
		bool isMouseOverGameViewport_ = false;
		void DrawEditorGridGfx();

		// Entity factory System members
		Framework::PrefabSystem prefabSystem_; // Each editor instance has its own prefab system
		PrefabType selectedPrefabType_ = PrefabType::None;
		std::string selectedPrefabName_;

		// --- Entity Selection & Deletion ---
		bool isEntitySelected_ = false;  // Flag to track if an entity is selected
		std::optional<std::string> entityClipboard;

		// Drag and Drop Support (Windows to Editor)
		std::vector<std::filesystem::path> droppedFilePaths_;
		static void DropCallback(GLFWwindow* window, int count, const char* paths[]);

		// --- Private Helper Functions ---
		
		// Internal Checkers
		void SanityCheck();

		/**
		 * @brief Converts the current mouse position from screen coordinates to world coordinates.
		 * @return A Vector2D representing the mouse's position in the game world's normalized device coordinates (-1 to 1).
		 */
		Vector2D GetMouseWorldPosition(); // maybe can be in inputsystem.h in the future

		/**
		 * @brief Handles placing the dragged prefab when the mouse is released.
		 */
		void HandleDragDropPlacement(); // Renamed from HandleTilePlacement	


		/**
		 * @brief Handles logic for erasing a tile from the grid.
		 */
		void HandleTileErasing();


		/**
		 * @brief Handles the logic for spawning a selected prefab at the mouse cursor position.
		 */
		void HandleSpawning();

		/**
		 * @brief Handles the logic for selecting an entity by clicking on it in the game world.
		 */
		void HandleSelection();
		void HandleDeletion();

		/* Entity Copy & Pasting APIs */
		void CopyEntityToClipboard(const Entity& entity);
		void PasteEntityFromClipboard();

		// --- GUI Panel Functions ---
		void ShowPerformanceProfile();
		void ShowLevelEditor();
		void SaveAndLoadSection(); // within LevelEditor panel
		void ShowHierarchy();
		void ShowInspector();
		void ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent);
		void ShowTilemapEditor();
		void ShowAssetBrowser(std::vector<std::filesystem::path>& droppedFiles);
		void ShowGameViewport();
		void ShowToolbar();
		void ShowAIDebugPanel(); 
		// --- Grid Helper Functions ---
		void DrawEditorGrid(ImDrawList* drawList); // MODIFIED: Added ImDrawList*
		Vector2D SnapToGrid(const Vector2D& worldPos);
		void ResetPlayerToDefaultPosition();


		// --- NEW Helper ---

		 /**
		 * @brief Renders a preview of the tile being dragged.
		 */
		void DrawDragDropPreview(ImDrawList* drawList); //  Function to draw preview

		/**
	    * @brief Loads textures and prepares palette items.
		*/
		void PreparePaletteItems();

		
		
		//ImVec2 WorldToImGuiScreen_Absolute(const Vector2D& worldPos);

		// Caches the game viewport's position and size
		void UpdateViewportData();

	public:
		bool isEnabled = true;
		EditorSettings settings;

		/* Constructors */
		// Editors should not be cloneable or assignable. Only 1 should ever exist on the engine.
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm);

		/**
		 * @brief Initializes the Editor system.
		 * @details Loads prefabs from file and sets up the ImGui context and backends.
		 */
		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override;

		// Converts world-space coordinates to ImGui screen-space
		ImVec2 WorldToImGuiScreen(const Vector2D& worldPos);
	};
}