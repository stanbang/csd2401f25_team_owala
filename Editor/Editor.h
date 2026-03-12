/*********************************************************************//**
* \file   Editor.h
*
* \primary_author
*		Jason Lou (16.66%)
* \secondary_author
*		Lur Pei Hao Stanley (16.66%)
*		Sitara Anil (16.66%)
*		Ng Jordain Ying Zhe (16.66%)
*		Yar Xuan (16.66%)
*		Ng Jing Wen (16.66%)
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
#include "PhysicsSystem.h"
#include "TransformationSystem.h"
#include "LayerManager.h"

// Editor Modules
#include "History.h"
#include "Gizmo.h"
#include "ProjectScaffold.h"

// Imgui Dependencies
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "AssetBrowser.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <optional>

namespace Framework {
	class GameManager; // Forward Declaration
	class CoreEngine; // Forward Declaration
	class CameraSystem;
	struct SystemTime; // Forward Declaration
	//class PhysicsSystem; // forward Declaration
}

namespace Editor {
	// Helper function to convert component type to string
	std::string ComponentToString(Framework::ComponentType componentType);


	struct EditorSettings {
		int windowWidth = 1920;
		int windowHeight = 1080;

		bool showDebugInfo = true;
		bool showLevelEditor = true;
		bool showHierarchy = true;
		bool showInspector = true;
		bool showGameViewport = true;
		bool showAssetBrowser = true;
		bool showToolbar = true;
		bool showPreferencesWindow = false;
		bool showProjectSettingsWindow = false;
		bool showPrefabEditor = false;
		bool showLayerManager = true;
		bool showAnimationEditor = false;
	};

	// enum to represent the different types of objects we can create
	enum class PrefabType {
		None,
		Eraser,
		Platform,
		TrapPlatform,
		Tile
	};

	// Structure to hold info about a draggable prefab for the palette
	struct DraggablePrefabInfo {
		std::string name;
		PrefabType type = PrefabType::None;
		Framework::Texture textureID = 0; // OpenGL Texture ID
		// tex coords(uses dearimgui vec struct)
		ImVec2 uv0{ 0.0f, 0.0f };
		ImVec2 uv1{ 1.0f, 1.0f };
	};

	// Helper Struct for Tile Data
	struct TileData {
		std::string name;
		std::string textureID;
	};

	
	// struct for draggable fsm items (enemys)
	struct DraggableFSMInfo {
		std::string name;
		std::string fsmPath;
		std::string textureKey;
		ImTextureID textureID = 0;
		ImVec2 uv0;
		ImVec2 uv1;
		ImVec2 halfSize{ 25.0f, 25.0f };
	};

	// Struct for handling Editing Session of the Inspector Window for the HistoryManager.
	struct EditSession {
		bool active = false;
		bool wasItemActive = false;
		Entity entityInEdit = 0;
		Framework::ComponentType compInEdit = Framework::ComponentType::MAX_COMPONENTS;
		std::string beforeJSON;

		// Marks the beginning of the editing session for a component, saving it's initial state.
		void Begin(Entity entity, Framework::ComponentType compType);
		// Marks the end of the editing session for a component, saving it's post modification state.
		void End(Entity entity, Framework::ComponentType compType, const std::string& description, HistoryManager& historyManagerRef);
		// Clears Session Data.
		void Flush();
	};

	class Editor : public Framework::ISystem {
	private:
		// --- System Pointers ---
		Framework::CoreEngine* engine_ = nullptr; // Enforce that the Editor must be instantiated after engine
		Framework::GameManager* gameManager_ = nullptr; // Enforce that the Editor must be instantiated after GameManager
		Framework::CameraSystem* cameraSystem_ = nullptr; // Pointer to camera systems for level editor
		Framework::Graphics* graphicsSystem_ = nullptr; //  Pointer to Graphics system
		Framework::ResourceManager* resourceManager_ = nullptr; // Pointer to ResourceManager for asset loading
		Framework::TransformationSystem* transformationSystem_ = nullptr;

		// --- Codependent Systems ---
		HistoryManager historyManager_; // HistoryManager for Undo/Redo System.
		GizmoSystem* gizmoSystem_ = nullptr;

		// --- Internal Properties ---
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

		// Entity Inspector
		EditSession editSession;
		int selectedEntityIndex = 0;
		std::vector<std::string> componentNames;
		std::vector<const char*> componentItems;
		std::vector<Framework::ComponentType> componentTypes;
		int selectedComponentIndex = 0;
		std::vector<std::string> tagNames;
		std::vector<const char*> tagItems;
		std::vector<Framework::ComponentType> tagTypes;
		int selectedTagIndex = 0;
		// Native Scripting
		int selectedScriptIndex = 0;
		int selectedReplaceScriptIndex = 0;

		// --- Tilemap Editor State ---
		bool isTileEditorActive_ = false; //  Flag to control tile editor mode
		bool showGrid_ = false;
		float baseGridCellSize_ = 100.f;   // NEW: World-space size of a cell at zoom 1.0

		//Drag and Drop State
		bool isDraggingPrefab_ = false;
		DraggablePrefabInfo dragDropPayload_; // Holds info about the prefab being dragged
		Vector2D dragDropPreviewPos_; // World position for preview rendering

		// Tracks whether move game object mode is activated. Toggled by Toggle Move button on the toolbar, for in world game obj drag and drop
		bool isDraggingGameObj_ = false; 

		// Brush mode for continous placement
		bool isBrushMode_ = false;
		DraggablePrefabInfo brushPayload_; // Stores the item selected for brushing
		Vector2D brushPreviewPos_;         // World position for brush preview
		Vector2D lastPlacedPos_ = { -99999.0f, -99999.0f }; // To prevent stacking duplicates on same frame

		//Palette Info storage (prefabs)
		bool palettePrepared_ = false; // flag for storing palette items
		std::vector<DraggablePrefabInfo> paletteItems_;

		//tiles
		bool tilePalettePrepared_ = false;
		bool isTilePaletteOpen_ = false;
		std::vector<DraggablePrefabInfo> tilePaletteItems_;
		std::vector<TileData> tileDataList_;

		//---layer manager variables ----//
		Framework::LayerManager layerManager_;

		// Layer editor state
		bool showLayerConfigWindow_ = false;
		int selectedRenderLayerIdx_ = -1;
		int selectedCollisionLayerIdx_ = -1;
		char newRenderLayerName_[128] = "New Layer";
		int newRenderLayerOrder_ = 0;
		char newCollisionLayerName_[128] = "New Layer";

		// Game viewport tracking for proper mouse coordinate transformation
		ImVec2 gameViewportPos_ = { 0, 0 };    // Top-left position of game viewport in screen space
		ImVec2 gameViewportSize_ = { 0, 0 };   // Size of game viewport
		bool isMouseOverGameViewport_ = false;
		//void DrawEditorGridGfx();

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

		// FSM Editor state
		bool isFSMEditorOpen_ = false;
		std::string selectedFSMDefinition_;
		std::vector<std::string> availableFSMDefinitions_;
		bool fsmPalettePrepared_ = false;

		std::vector<DraggableFSMInfo> fsmPaletteItems_;
		DraggableFSMInfo fsmDragDropPayload_;
		bool isDraggingFSM_ = false;

		// --- Camera State Management ---
		struct CameraState {
			Vector2D position{ 0.0f, 0.0f };
			float zoom = 1.0f;
		};

		CameraState defaultEditorCameraState_{ {0.0f, 0.0f}, 1.0f };
		CameraState currentEditorCameraState_;
		bool editorCameraNeedsReset_ = false;

		// -------------------------------------------------------
		// --- Project Management State (New / Import Project) ---
		// -------------------------------------------------------
		bool showNewProjectDialog_ = false;  // Controls "Create New Project" modal visibility
		bool showImportProjectDialog_ = false;  // Controls "Import Existing Project" modal visibility
		char newProjectNameBuffer_[256] = "";  // Text input buffer for the new project name
		char newProjectPathBuffer_[512] = "";  // Root directory where the new project folder will be created
		char importProjectPathBuffer_[512] = ""; // Path to the folder of an existing project to import
		std::string projectFeedbackMsg_ = "";  // Feedback message shown inside the modal after an action
		bool projectActionSucceeded_ = false; // Whether the last project action succeeded (colours the message)

		// Renders the "Create New Project" modal pop-up.
		void ShowNewProjectDialog();

		// Renders the "Import Existing Project" modal pop-up.
		void ShowImportProjectDialog();
		// -------------------------------------------------------
		void ResetEditorCamera();
		void SaveEditorCameraState();
		void RestoreEditorCameraState();

		// --- Private Helper Functions ---

		// Internal Checkers
		void SanityCheck();

		// Editor Configuration
		void LoadEditorConfig();
		void SaveEditorConfig();

		// Handles placing the dragged prefab when the mouse is released.
		void HandleDragDropPlacement(); // Renamed from HandleTilePlacement	

		//  Renders a preview of the tile being dragged.
		void DrawDragDropPreview(ImDrawList* drawList);

		// Handles logic for erasing a tile from the grid.
		void HandleTileErasing();

		//  Handles the logic for spawning a selected prefab at the mouse cursor position.
		void HandleSpawning();
		
		// Handles the logic for selecting an entity by clicking on it in the game world.
		void HandleSelection();

		// Handles the logic for deleting an entity by clicking on it in the game world.
		void HandleDeletion();

		// Handle input keys when game viewport is active
		void HandleGameViewportInput();

		/* Entity Copy & Pasting APIs */
		void CopyEntityToClipboard(const Entity entity);
		void PasteEntityFromClipboard();
		std::string SerializeEntityToString(const Entity entity);

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
		void ShowMenuBar();
		void ShowToolbar();
		void ShowPreferencesWindow();
		void ShowProjectSettingsWindow();
		void ShowDebugVisualizationPanel();
		void ShowPrefabEditor();
		void ShowTilePalette();
		//void ShowRestartMenu(); //when player dies
		void ShowLayerManager();

		void DrawTransformInspector(Entity entity);
		void DrawRigidBodyInspector(Entity entity);
		void DrawTextureInspector(Entity entity);
		void DrawAnimationInspector(Entity entity);
		void DrawAudioInspector(Entity entity);
		void DrawButtonInspector(Entity entity);
		void DrawCameraInspector(Entity entity);

		// Popup Related
		std::string popupMsg;

		// --- Grid Helper Functions ---
		void DrawEditorGrid(ImDrawList* drawList); //uses imgui drawlist to draw onto screen
		//void ResetPlayerToDefaultPosition();

		std::string selectedPrefabForEditing_;
		PrefabType selectedPrefabTypeForEditing_ = PrefabType::None;
		bool isPrefabEditorOpen_ = false;

		// Helper functions for prefabs 
		void ApplyPrefabChangesToEntities(const std::string& prefabName);
		void SavePrefabToFile(const std::string& prefabName);
		void ApplyAllPrefabChangesToCurrentScene();

		// Animation Editor Helper functions
		void ShowAnimationEditor();
		void LoadAnimationFiles();
		void SaveAnimationFile(const std::string& animationName);
		void SaveTextureToAnimationMapping();
		void ReloadAnimationFromFile(const std::string& animationName);
		void RefreshAnimationInResourceManager(const std::string& animationName);

		// Scene Management Helper functions
		void SaveScene();

		// Returns entity at current mouse position in the game world, 0 if not found.
		Entity GetObjAtMousePos();

		// Loads textures and prepares palette items.
		void PreparePaletteItems();

		//void LoadTilesFromFile();
		void PrepareTilePaletteItems();

		// Function for placing items in world
		void PlaceObjectInWorld(const DraggablePrefabInfo& info, Vector2D position);

		// Helper to get the HalfSize of a prefab or tile BEFORE creating it
		Vector2D GetPrefabHalfSize(const std::string& name, PrefabType type);

		// Handle the continuous clicking input
		void HandleBrushInput();

		//ImVec2 WorldToImGuiScreen_Absolute(const Vector2D& worldPos);

		// Caches the game viewport's position and size
		void UpdateViewportData();


		// Sort a list of entities by descending render layer
		void SortEntitiesByRenderLayer(std::vector<Entity>& entities, Framework::EntityManager& entityMgr);


		// FSM DRAGGABLE HELPER FUNCTIONS
		void ShowFSMEditor();
		void PrepareFSMPaletteItems();
		void HandleFSMDragDropPlacement();
		void LoadFSMDefinitionsFromFolder(const std::string& folderPath);
		Entity CreateFSMEntityAtPosition(const std::string& fsmPath, const Vector2D& position);

		// Layer viewport data for IMGUI
		void ShowCollisionMatrix(Framework::PhysicsSystem* physics);
		void ShowEntityCollisionLayer(Framework::EntityManager& entityMgr, Entity selectedEntity);
		void ShowEntityRenderLayer(Framework::EntityManager& entityMgr, Entity selectedEntity);
		void ShowSceneEntityTracker(Framework::EntityManager& entityMgr);
		void ShowLayerConfigWindow();
		void ShowRenderLayersTab();
		void ShowCollisionLayersTab();


		// --- Animation Editor State ---
		struct AnimationFileData {
			std::string name;
			int frameCount = 0;
			float frameDuration = 0.1f;
			bool isLooping = true;
			int startFrame = 0;
			std::vector<std::pair<std::string, int>> spritesheets; // texture, frameCount pairs
			std::vector<std::string> textureKeys; // keys from textureToAnimationMapping.json
			std::map<int, std::string> frameEvents; // maps frame numbers to event names for quick lookup when editing events on specific frames
		};

		std::vector<AnimationFileData> loadedAnimations_;
		int selectedAnimationIndex_ = 0;
		std::string selectedAnimationName_;
		std::vector<const char*> animationNames_;
		bool isAnimationEditorInitialized_ = false;
		bool showCreatePrefabDialog_ = false;
		char newPrefabNameBuffer_[256] = "";
		Entity entityForPrefabCreation_ = 0;

		// Helper function
		void ShowCreatePrefabDialog();

	public:
		bool isEnabled = true;
		EditorSettings settings;

		/* Constructors */
		// Editors should not be cloneable or assignable. Only 1 should ever exist on the engine.
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm);

		// Editor loop functions
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

		/* Helper Functions for playing / pausing state */
		const bool IsGamePlaying() const { return isGamePlaying; };
		const bool IsGamePaused() const { return isGamePaused; };
		const ImVec2 GetGameViewportSize() const { return gameViewportSize_; }
		const ImVec2 GetGameViewportPos() const { return gameViewportPos_; }

		Vector2D SnapToGrid(const Vector2D& worldPos, bool normalized = false) const;

		
		//Converts the current mouse position from screen coordinates to world coordinates.
		Vector2D GetMouseWorldPosition(); // maybe can be in inputsystem.h in the future

		Vector2D GetNormalizedMousePos() const;

		// Converts world - space coordinates to ImGui screen - space
		// function that accepts a specific camera ID for movement
		ImVec2 WorldToImGuiScreen(const Vector2D& worldPos, Entity cameraID);

		// Converts world-space coordinates to ImGui screen-space
		ImVec2 WorldToImGuiScreen(const Vector2D& worldPos);
		// Converts normalized coordinates to ImGui screen-space
		ImVec2 NormalizedToImGuiScreen(const Vector2D& normalizedPos) const;

		// Setter for GizmosSystem
		void SetGizmoSystem(GizmoSystem* gizmo) {
			gizmoSystem_ = gizmo;
		}

		// Getter for historyManager_
		HistoryManager* GetHistoryMgr() { return &historyManager_; }
	};
}