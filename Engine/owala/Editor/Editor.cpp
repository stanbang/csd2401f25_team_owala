/*********************************************************************//**
* \file   Editor.cpp
*
* \primary_author
*      Jason Lou (50%)
* \secondary_author
*	   Lur Pei Hao Stanley (50%)
*
*
* \brief	Implementation of the Editor system for in-game level editing and
			debugging using ImGUI.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#include "Editor.h"
#include "Core.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "CameraSystem.h"
#include "Graphics.h"     // Include Graphics for drawing preview
#include "AssetBrowser.h"

// Debug
#include "Logger.h"

// Imgui Dependencies
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <random> // for random generation

#include "Fake_Player.h"

// Internal Helpers
namespace {
	using namespace Framework;

	std::string ComponentToString(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PlayerStatsType:				return "PlayerStats";
		case ComponentType::PositionType:					return "Position";
		case ComponentType::VelocityType:					return "Velocity";
		case ComponentType::AccelerationType:				return "Acceleration";
		case ComponentType::TriangleType:					return "Triangle";
		case ComponentType::RigidBodyType:					return "RigidBody";
		case ComponentType::AABBColliderType:				return "AABBCollider";
		case ComponentType::PlayerStateType:				return "PlayerState";
		case ComponentType::RotationFlagType:				return "RotationFlag";
		case ComponentType::TrapPlatformComponentType:		return "TrapPlatform";
		case ComponentType::ScalingPlatformComponentType:	return "ScalingPlatform";
		case ComponentType::TextureComponentType:			return "Texture";
		case ComponentType::AudioComponentType:				return "Audio";
		case ComponentType::AnimationComponentType:			return "Animation";
		case ComponentType::TextComponentType:				return "Text";
		case ComponentType::PlayerTagType:					return "PlayerTag";
		case ComponentType::PlatformTagType:				return "PlatformTag";
		case ComponentType::BackgroundTagType:				return "BackgroundTag";
		case ComponentType::EnemyTagType:					return "EnemyTag";
		case ComponentType::SceneOwnedType:					return "SceneOwned";
		case ComponentType::PersistentTagType:				return "PersistentTag";
		case ComponentType::ActiveTagType:					return "ActiveTag";
		default:											return "Unknown";
		}
	}

	// Helper to convert a CollisionLayer enum to its string representation.
	const char* LayerToString(CollisionLayer layer) {
		switch (layer) {
		case CollisionLayer::Default:   return "Default";
		case CollisionLayer::Player:    return "Player";
		case CollisionLayer::Platform:  return "Platform";
		case CollisionLayer::Enemy:     return "Enemy";
		case CollisionLayer::Trigger:   return "Trigger";
		default:                        return "Invalid";
		}
	}
}

namespace Editor {
	// --- Constructor ---
	Editor::Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm) : engine_(&engine), gameManager_(&gameManager), resourceManager_(&rcm), assetBrowser(rcm), 
		cameraSystem_(gameManager_->GetSystems().camera_), // for tile editor to work with camera
		graphicsSystem_(gameManager_->GetSystems().gfx_) { // for drag drop and placing of prefabs
		
	};	

	void Editor::SanityCheck() {
		using namespace Framework;

		if (!engine_) {
			throw GameException(DebugType::Editor, "What the sigma, how are you running this without an engine???");
		}

		if (!gameManager_) {
			throw GameException(DebugType::Editor, "GameManager not registered!");
		}

		if (!gameManager_->GetSystems().camera_) {
			throw GameException(DebugType::Editor, "CameraSystem not registered with GameManager!");
		}

		if (!gameManager_->GetSystems().resourceManager_) {
			throw GameException(DebugType::Editor, "Resource Manager not registered with GameManager!");
		}

		if (!SceneManager::Exists()) {
			throw GameException(DebugType::Editor, "Missing SceneManager!");
		}

		if (!initialized) {
			Initialize(); // Re-initialize if it has not been initialized somehow
		}
	}

	// ----------------------------------------------------------------------------
	// Initializes the Editor system
	// Loads prefabs from file and sets up the ImGui context and backends.
	// ----------------------------------------------------------------------------
	void Editor::Initialize() {
		if (initialized || !isEnabled) return; // Skip initialization if Editor is not enabled or has already initialized


		// --- IMGUI INITIALIZATION ---
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking;     
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled, we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(gameManager_->GetSystems().win_->window(), true);
		ImGui_ImplOpenGL3_Init("#version 330");

		GLFWwindow* window = gameManager_->GetSystems().win_->window();

		// GLFW Drag and Drop callback
		glfwSetWindowUserPointer(window, this);
		glfwSetDropCallback(window, Editor::DropCallback);

		// Init Asset Browser to root Engine directory
		fs::path exePath = fs::current_path();
		fs::path rootPath = fs::absolute(exePath / "../../"); //Moves out of Debug, then out of build, to reach Engine root
		assetBrowser.Initialize(rootPath);

		// Prepare Dropdown arrays for Entity Inspector
		componentNames.clear();
		componentItems.clear();
		componentTypes.clear();
		tagNames.clear();
		tagItems.clear();
		tagTypes.clear();

		componentNames.push_back("");
		tagNames.push_back("");

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

		for (int i = 0; i < static_cast<int>(ComponentType::MAX_COMPONENTS); ++i)
		{
			ComponentType type = static_cast<ComponentType>(i);
			if (!entityMgr.IsTagComponent(type))
			{
				componentNames.push_back(ComponentToString(type));
				componentTypes.push_back(type);
			}
			else {
				if (type == ComponentType::ActiveTagType) continue; // Skip ActiveTag since user will not be able to manually add/remove it.
				tagNames.push_back(ComponentToString(type));
				tagTypes.push_back(type);
			}
		}

		for (const std::string& componentName : componentNames) {
			componentItems.push_back(componentName.c_str());
		}

		for (const std::string& tagName : tagNames) {
			tagItems.push_back(tagName.c_str());
		}

		// Load all prefabs from the data file when the editor starts
		// which populates our prefabSystem_ member with spawnable objects
		if (!prefabSystem_.LoadPrefabsFromFile("Data/platformPrefab.txt")) {
			// Log an error if the prefab file can't be found or is invalid.
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to load prefabs from Data/platformPrefab.txt" << std::endl;
		}

		// --- Prepare Palette Items (Load Textures) ---

		initialized = true;
	}

	// Drag and drop callback function
	void Editor::DropCallback(GLFWwindow* window, int count, const char* paths[])
	{
		Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
		if (editor)
		{
			editor->droppedFilePaths_.clear();
			for (int i = 0; i < count; ++i)
			{
				editor->droppedFilePaths_.push_back(paths[i]);
			}
		}
	}

	void Editor::Update(float dt) {

		//Framework::Logger::Get().Info(DebugType::Editor)
		//	<< "Editor::Update Start Frame. palettePrepared_: " << (palettePrepared_ ? "true" : "false")
		//	<< ", resourceManager_: " << (resourceManager_ ? "valid" : "null") << std::endl;
		//// *** END LOGGING BLOCK ***

		if (!palettePrepared_ && resourceManager_) {
			Framework::Logger::Get().Info(DebugType::Editor) << "Running PreparePaletteItems..." << std::endl; // Log before running
			PreparePaletteItems();
			palettePrepared_ = true;
			// Log the size AFTER preparing
			Framework::Logger::Get().Info(DebugType::Editor) << "PreparePaletteItems finished. paletteItems_ size: " << paletteItems_.size() << std::endl;
		}

		deltatime = dt;

		// Check if Editor is enabled
		if (!isEnabled) return;

		// Check if Editor has been initialized
		SanityCheck();

		
		//  --- START THE IMGUI FRAME ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// IMGui Cursor State
		ImGuiIO& io = ImGui::GetIO();

		bool imguiHasFocus = io.WantCaptureMouse || io.WantCaptureKeyboard ||
			ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

		// Always keep cursor unlocked while editor is active
		gameManager_->GetSystems().input_->SetCursorLocked(false);

		// --- Handle Editor Logic ---
		HandleSpawning();  // Check if we should spawn a selected prefab
		HandleSelection(); // Check if the user is clicking on an entity in the world
		HandleDeletion();  // Check if the user is deleting the selected entity
		
		// ---  Draw the editor grid ---
		if (showGrid_) {
			DrawEditorGrid();
		}

		// --- NEW: Draw Drag Preview ---
		if (isDraggingPrefab_) {
			DrawDragDropPreview();
		}

		// --- Handle Editor Logic ---
		// Placement/Erasing only happen if tile editor is active
		if (isTileEditorActive_) {
			// Check for drop first (mouse release)
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				HandleDragDropPlacement(); // Try to place if dragging
			}
			HandleTileErasing(); // Eraser might still be click/drag based
		}
		else {
			// Selection only happens if tile editor is NOT active
			HandleSelection();
		}
		// Deletion can happen anytime if an entity is selected
		HandleDeletion();
		
		 // Reset drag state if mouse is released anywhere
		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			isDraggingPrefab_ = false;
			// dragDropPayload_ is cleared inside HandleDragDropPlacement or naturally when drag ends
		}

		// --- RENDER PANELS ---
		//Render Asset Browser
		ShowAssetBrowser(droppedFilePaths_);
		
		// Render all editor UI panels

		// Level Editor Window
		if (settings.showLevelEditor) {
			ShowLevelEditor();
		}

		// Performance Profiler Window
		if (settings.showDebugInfo) {
			ShowPerformanceProfile();
		}

		
	//	if (isEntitySelected_) ShowPlayerInspector(); // Show inspector if an entity is selected

		//  Show Tilemap Editor as a separate window if active
		if (isTileEditorActive_) {
			ShowTilemapEditor();
		}

		// --- END IMGUI FRAME ---

		// ----------------------------------------------------------------------------
		// Finalize the ImGui frame and render all its draw data.
		// ----------------------------------------------------------------------------
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	// ----------------------------------------------------------------------------
	// Clean up ImGui resources when the application closes.
	// ----------------------------------------------------------------------------
	void Editor::Shutdown() {
		std::cout << "Freeing IMGUI" << std::endl;
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}


	// --- Private Helper Functions ---

	// NEW Helper: Load textures and create palette items
	void Editor::PreparePaletteItems() {
		paletteItems_.clear();

		// Helper lambda to add an item if texture is found
		auto AddPaletteItem = [&](const std::string& name, PrefabType type, const std::string& textureKey) {
			if (textureKey.empty()) {
				Framework::Logger::Get().Error(DebugType::Editor) << "Skipping palette item '" << name << "' due to empty texture key." << std::endl;
				return; // Don't try to add if key is empty
			}
			try {
				// Log before attempting to get the texture
				Framework::Logger::Get().Info(DebugType::Editor) << "Attempting to get texture for palette item: '" << name << "' with key: '" << textureKey << "'" << std::endl;

				// Ensure resourceManager_ pointer is valid
				if (!resourceManager_) {
					Framework::Logger::Get().Error(DebugType::Editor) << "ResourceManager pointer is null in AddPaletteItem!" << std::endl;
					return;
				}

				// Get texture ID from ResourceManager
				Texture texID = resourceManager_->get<Texture>(textureKey);

				if (texID != 0) {
					paletteItems_.push_back({ name, type, texID });
					Framework::Logger::Get().Info(DebugType::Editor) << " -> Success! Added palette item: '" << name << "' (ID: " << texID << ")" << std::endl;
				}
				else {
					// This case might happen if loadTexture returned 0 but didn't throw
					Framework::Logger::Get().Error(DebugType::Editor) << " -> Failed! Texture ID is 0 for palette item: '" << name << "' (Key: '" << textureKey << "')" << std::endl;
				}
			}
			catch (const std::out_of_range&) {
				// This specifically means the key wasn't found in the map
				Framework::Logger::Get().Error(DebugType::Editor) << " -> Failed! Texture key '" << textureKey << "' not found in ResourceManager for palette item: '" << name << "'" << std::endl;
			}
			catch (const std::exception& e) { // Catch other standard exceptions
				Framework::Logger::Get().Error(DebugType::Editor) << " -> Failed! Exception getting texture for palette item '" << name << "' (Key: '" << textureKey << "'): " << e.what() << std::endl;
			}
			catch (...) {
				Framework::Logger::Get().Error(DebugType::Editor) << " -> Failed! Unknown error getting texture for palette item: '" << name << "' (Key: '" << textureKey << "')" << std::endl;
			}
			};

		// Add Eraser (no texture needed, or use a placeholder if desired)
		// paletteItems_.push_back({"Eraser", PrefabType::Eraser, 0}); // Add eraser without texture

		// Add items for each prefab type
		for (const auto& pair : prefabSystem_.GetPlatformPrefabs()) {
			AddPaletteItem(pair.first, PrefabType::Platform, pair.second.textureID);
		}
		for (const auto& pair : prefabSystem_.GetTrapPlatformPrefabs()) {
			AddPaletteItem(pair.first, PrefabType::TrapPlatform, pair.second.textureID);
		}
		for (const auto& pair : prefabSystem_.GetScalingPlatformPrefabs()) {
			AddPaletteItem(pair.first, PrefabType::ScalingPlatform, pair.second.textureID);
		}
		Framework::Logger::Get().Info(DebugType::Editor) << "Prepared " << paletteItems_.size() << " palette items." << std::endl;
	}


	Vector2D Editor::GetMouseWorldPosition() {
		auto* input = gameManager_->GetSystems().input_;
		auto* win = gameManager_->GetSystems().win_;

		if (!input || !win || !cameraSystem_) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Missing required systems in GetMouseWorldPosition" << std::endl;
			return { 0.0f, 0.0f }; // Return zero vector if systems are missing
		}

		// Get active camera
		Entity activeCamera = cameraSystem_->GetActiveCamera();
		if (activeCamera == 0) {
			Framework::Logger::Get().Error(DebugType::Editor) << "No active camera in GetMouseWorldPosition" << std::endl;
			return { 0.0f, 0.0f };
		}

		double mouseX, mouseY;
		input->GetMousePos(&mouseX, &mouseY);

		// Convert screen pixel coordinates (0,0 top-left) to NDC (-1 to 1, origin center)
		float ndcX = (static_cast<float>(mouseX) / win->getWidth()) * 2.0f - 1.0f;
		float ndcY = 1.0f - (static_cast<float>(mouseY) / win->getHeight()) * 2.0f; // Y is inverted

		// Convert NDC to World using the camera system
		return cameraSystem_->ScreenToWorld({ ndcX, ndcY });
	}

	void Editor::HandleDragDropPlacement() {
		// Check if we were actually dragging a prefab and if mouse is released over game view
		if (!isDraggingPrefab_ || ImGui::GetIO().WantCaptureMouse) {
			isDraggingPrefab_ = false; // Ensure dragging stops if mouse released over UI
			return;
		}

		Vector2D worldMousePos = GetMouseWorldPosition();
		Vector2D snappedPos = SnapToGrid(GetMouseWorldPosition());
		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		// *** ADD Log for payload check ***
		Framework::Logger::Get().Info(DebugType::Editor)
			<< "HandleDragDropPlacement: Payload Name='" << dragDropPayload_.name
			<< "', Payload Type=" << static_cast<int>(dragDropPayload_.type) << std::endl;

		// Check for existing tile (using current grid size for tolerance)
		const float placementTolerance = baseGridCellSize_ * 0.1f;

		for (const auto& entity : sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name)) {
			if (!entityMgr.HasComponent(entity, Framework::PlatformTagType)) continue;
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const auto& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				if (std::abs(pos.x - snappedPos.x) < placementTolerance && std::abs(pos.y - snappedPos.y) < placementTolerance) {
					Framework::Logger::Get().Info(DebugType::Editor) << "Tile already exists at placement location." << std::endl;
					isDraggingPrefab_ = false; // Stop drag operation
					return;
				}
			}
		}

		// Create new entity and add components manually to control size
		Entity newEntity = entityMgr.CreateEntity();
		// Use the dynamically calculated currentGridCellSize_ for square tiles
		Vector2D tileHalfSize = { baseGridCellSize_ / 2.0f, baseGridCellSize_ / 2.0f };
		
		// Common components for all platforms
		entityMgr.AddComponent(newEntity, Framework::PlatformTag{});
		entityMgr.AddComponent(newEntity, Framework::Position{ snappedPos });
		entityMgr.AddComponent(newEntity, Framework::Velocity{});
		entityMgr.AddComponent(newEntity, Framework::RigidBody{}); // Add RigidBody first
		entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(0.0f); // Set default mass
		entityMgr.AddComponent(newEntity, Framework::AABBCollider{ tileHalfSize });
		entityMgr.AddComponent(newEntity, Framework::RotationFlag{});
		//entityMgr.GetProperties(newEntity).layer = CollisionLayer::Platform;
		entityMgr.GetProperties(newEntity) = { "platforms", CollisionLayer::Platform };
		

		// Type-specific components using info from dragDropPayload_
		std::string textureKey = "";
		if (dragDropPayload_.type == PrefabType::Platform) {
			Framework::Logger::Get().Info(DebugType::Editor) << "If/Else: Matched Platform" << std::endl; // Add log
			const auto* prefab = prefabSystem_.GetPlatformPrefab(dragDropPayload_.name);
			if (prefab) {
				Framework::Logger::Get().Info(DebugType::Editor) << " -> Found Platform Prefab. Texture ID: '" << prefab->textureID << "'" << std::endl;
				textureKey = prefab->textureID;
				if (prefab->mass > 0.0f) entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(prefab->mass);
			}
			else {
				Framework::Logger::Get().Error(DebugType::Editor) << " -> FAILED to find Platform Prefab: '" << dragDropPayload_.name << "'" << std::endl;
			}
		}
		else if (dragDropPayload_.type == PrefabType::TrapPlatform) {
			Framework::Logger::Get().Info(DebugType::Editor) << "If/Else: Matched TrapPlatform" << std::endl; // Add log
			const auto* trapPrefab = prefabSystem_.GetTrapPlatformPrefab(dragDropPayload_.name);
			if (trapPrefab) {
				Framework::Logger::Get().Info(DebugType::Editor) << " -> Found TrapPlatform Prefab. Texture ID: '" << trapPrefab->textureID << "'" << std::endl;
				textureKey = trapPrefab->textureID;
				Framework::TrapPlatformComponent trapComp;
				trapComp.originalPosition = snappedPos;
				trapComp.fallSpeed = trapPrefab->fallSpeed;
				trapComp.triggerDelay = trapPrefab->triggerDelay;
				entityMgr.AddComponent(newEntity, trapComp);
				if (trapPrefab->mass > 0.0f) entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(trapPrefab->mass);
			}
			else {
				Framework::Logger::Get().Error(DebugType::Editor) << " -> FAILED to find TrapPlatform Prefab: '" << dragDropPayload_.name << "'" << std::endl;
			}
		}
		else if (dragDropPayload_.type == PrefabType::ScalingPlatform) {
			Framework::Logger::Get().Info(DebugType::Editor) << "If/Else: Matched ScalingPlatform" << std::endl; // Add log
			const auto* scalePrefab = prefabSystem_.GetScalingPlatformPrefab(dragDropPayload_.name);
			if (scalePrefab) {
				Framework::Logger::Get().Info(DebugType::Editor) << " -> Found ScalingPlatform Prefab. Texture ID: '" << scalePrefab->textureID << "'" << std::endl;
				textureKey = scalePrefab->textureID;
				Framework::ScalingPlatformComponent scaleComp;
				scaleComp.baseHalfSize = tileHalfSize;
				scaleComp.scaleSpeed = scalePrefab->scaleSpeed;
				scaleComp.minScale = scalePrefab->minScale;
				scaleComp.maxScale = scalePrefab->maxScale;
				scaleComp.currentScaleX = 1.0f; scaleComp.currentScaleY = 1.0f;
				entityMgr.AddComponent(newEntity, scaleComp);
				if (scalePrefab->mass > 0.0f) entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(scalePrefab->mass);
			}
			else {
				Framework::Logger::Get().Error(DebugType::Editor) << " -> FAILED to find ScalingPlatform Prefab: '" << dragDropPayload_.name << "'" << std::endl;
			}
		}
		// Optional: Add an else for Eraser/None if needed, or handle unexpected values
		else {
			Framework::Logger::Get().Error(DebugType::Editor)
				<< "If/Else block received UNEXPECTED PrefabType: "
				<< static_cast<int>(dragDropPayload_.type) << std::endl;
		}

		// Assign the texture key
		if (!textureKey.empty()) {
			Framework::Logger::Get().Info(DebugType::Editor) << "Assigning Texture Key: '" << textureKey << "'" << std::endl;
			// *** Get the calculated tileHalfSize we used for the collider ***
			tileHalfSize = { baseGridCellSize_ / 2.0f, baseGridCellSize_ / 2.0f };

			// *** ADD TextureComponent HERE, INCLUDING the HalfSize ***
			entityMgr.AddComponent(newEntity, Framework::TextureComponent{ textureKey, tileHalfSize }); // Pass tileHalfSize here

			// Now add to scene
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
			Framework::Logger::Get().Info(DebugType::Editor) << "Placed tile: " << dragDropPayload_.name << " at (" << snappedPos.x << ", " << snappedPos.y << ")" << std::endl;
		}
		else {
			Framework::Logger::Get().Error(DebugType::Editor) << "Could not find texture key for placed prefab: " << dragDropPayload_.name << std::endl;
			// Destroy the entity if texture is missing? Or leave it invisible?
			// sceneMgr.RemoveEntityFromScene(newEntity);
		}

		// --- Crucial: Reset drag state ---
		isDraggingPrefab_ = false;
		// Optionally clear dragDropPayload_ if needed, but it will be overwritten on next drag start
		// dragDropPayload_ = {};
	}

	// Draws a preview of the tile being dragged
	void Editor::DrawDragDropPreview() {
		// Ensure graphics system is available and we are dragging a valid prefab
		if (!graphicsSystem_ || !isDraggingPrefab_ || dragDropPayload_.type == PrefabType::None || dragDropPayload_.type == PrefabType::Eraser) {
			return;
		}

		// Get current mouse position and snap it
		Vector2D worldMousePos = GetMouseWorldPosition();
		dragDropPreviewPos_ = SnapToGrid(worldMousePos); // Store for potential placement
		
		// Get texture ID from the payload
		Texture previewTextureID = dragDropPayload_.textureID;
		if (previewTextureID == 0) return; // Don't draw if texture invalid

		// Calculate size based on current grid cell size
		Vector2D previewHalfSize = { baseGridCellSize_ / 2.0f, baseGridCellSize_ / 2.0f };
		
		Entity activeCamera = cameraSystem_->GetActiveCamera();
		if (activeCamera == 0) return; // No camera, can't draw

		// Convert world center to screen center
		Vector2D screenCenter = cameraSystem_->WorldToScreen(dragDropPreviewPos_, activeCamera);
		
		// Convert world corners to screen space to get screen-space size
		Vector2D worldTopLeft = { dragDropPreviewPos_.x - previewHalfSize.x,
								  dragDropPreviewPos_.y - previewHalfSize.y };
		Vector2D worldBottomRight = { dragDropPreviewPos_.x + previewHalfSize.x,
									  dragDropPreviewPos_.y + previewHalfSize.y };

		Vector2D screenTopLeft = cameraSystem_->WorldToScreen(worldTopLeft, activeCamera);
		Vector2D screenBottomRight = cameraSystem_->WorldToScreen(worldBottomRight, activeCamera);

		// Calculate screen-space size
		Vector2D screenSize = {
			screenBottomRight.x - screenTopLeft.x,
			screenBottomRight.y - screenTopLeft.y
		};

		// Draw using Graphics system's drawSprite
		// Note: drawSprite currently doesn't support alpha tint easily.
		// We might need to modify drawSprite or use a dedicated preview shader later.
		// For now, just draw it normally. A simple transparency could be added
		// by modifying the shader or using glColor4f if not using shaders directly.
		try {
			// Need a non-const Texture reference for drawSpriteEasy/drawSprite
			Texture tempTexID = previewTextureID;

			graphicsSystem_->drawSpriteEasy(tempTexID,
				screenTopLeft,     // Top-left in NDC
				screenSize,        // Size in NDC
				0.0f,              // Rotation
				{ dragDropPayload_.uv0.x, dragDropPayload_.uv0.y }, // min UV
				{ dragDropPayload_.uv1.x, dragDropPayload_.uv1.y }  // max UV
			);

			// TODO: Add alpha blending specifically for the preview?
			// graphicsSystem_->setAlphaBlend(true); // Ensure blending is on
			// ... draw call ...
			// Need a way to tint alpha, maybe modify shader or drawSprite
		}
		catch (const std::exception& e) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Error drawing preview: " << e.what() << std::endl;
		}
	}

	void Editor::HandleTileErasing() {
		auto* input = gameManager_->GetSystems().input_;

		if (dragDropPayload_.type != PrefabType::Eraser) return; // Check payload type now
		// Use MouseDown for continuous erasing
		if (!input->MouseDown(Framework::InputSystem::MouseButton::Left) || ImGui::GetIO().WantCaptureMouse) return;

		Vector2D worldMousePos = GetMouseWorldPosition();
		Vector2D snappedPos = SnapToGrid(worldMousePos);
		Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		Entity entityToErase = 0;
		const float eraseTolerance = baseGridCellSize_ * 0.1f;

		for (const auto& entity : sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name)) {
			if (!entityMgr.HasComponent(entity, Framework::PlatformTagType)) continue;
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const auto& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				if (std::abs(pos.x - snappedPos.x) < eraseTolerance && std::abs(pos.y - snappedPos.y) < eraseTolerance) {
					entityToErase = entity;
					break;
				}
			}
		}

		if (entityToErase != 0) {
			sceneMgr.RemoveEntityFromScene(entityToErase);
		}
	}

	//void Editor::HandleSpawning() {
	//	auto* input = gameManager_->GetSystems().input_;

	//	// Spawn on left click
	//	if (selectedPrefabType_ != PrefabType::None && input->MousePressed(Framework::InputSystem::MouseButton::Left)) {
	//		if (!ImGui::GetIO().WantCaptureMouse) {

	//			Vector2D spawnPos = GetMouseWorldPosition();
	//			Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
	//			Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
	//			Entity newEntity = 0;

	//			switch (selectedPrefabType_) {
	//			case PrefabType::Platform:
	//				newEntity = prefabSystem_.CreatePlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
	//				break;
	//			case PrefabType::TrapPlatform:
	//				newEntity = prefabSystem_.CreateTrapPlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
	//				break;
	//			case PrefabType::ScalingPlatform:
	//				newEntity = prefabSystem_.CreateScalingPlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
	//				break;
	//			case PrefabType::None: // Should not happen 
	//				break;
	//			}

	//			if (newEntity != 0) {
	//				sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
	//			}

	//			selectedPrefabType_ = PrefabType::None;
	//			selectedPrefabName_.clear();
	//		}
	//	}

	//	// Cancel on right click
	//	if (selectedPrefabType_ != PrefabType::None && input->MousePressed(Framework::InputSystem::MouseButton::Right)) {
	//		selectedPrefabType_ = PrefabType::None;
	//		selectedPrefabName_.clear();
	//	}
	//}

	
	void Editor::HandleSelection() {
		Framework::InputSystem* input = gameManager_->GetSystems().input_;

		// Only check for selection if a prefab is NOT being spawned
		if (selectedPrefabType_ != PrefabType::None) return;

		// We only check for selection on a new mouse click.
		// We also check 'WantCaptureMouse' to ensure the click wasn't on an ImGui window.
		if (!input->MousePressed(Framework::InputSystem::MouseButton::Left) || ImGui::GetIO().WantCaptureMouse) {
			return;
		}

		Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Vector2D mouseWorldPos = GetMouseWorldPosition();
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		Entity bestCandidate = 0;
		bool found = false;

		// Get the list of entities from the active scene 
		const std::vector<Entity>& activeEntities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

		// Iterate through entities to find which one was clicked.
		// iterate in reverse to correctly select the entity that is rendered on top.
		for (std::vector<Entity>::const_reverse_iterator it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
			const Entity& entity = *it;
			if (entityMgr.HasComponent(entity, Framework::ComponentType::PositionType) &&
				entityMgr.HasComponent(entity, Framework::ComponentType::AABBColliderType))
			{
				Vector2D& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				Vector2D& halfSize = entityMgr.GetComponent<Framework::AABBCollider>(entity).HalfSize;

				// check using AABB.
				if (mouseWorldPos.x >= pos.x - halfSize.x && mouseWorldPos.x <= pos.x + halfSize.x &&
					mouseWorldPos.y >= pos.y - halfSize.y && mouseWorldPos.y <= pos.y + halfSize.y)
				{
					bestCandidate = entity;
					found = true;
					break; // Stop after finding the top-most one
				}
			}
		}
		// Update the editor's selection state
		if (found) {
			selectedEntityIndex = bestCandidate;
			selectedEntityIndex += 1;
			isEntitySelected_ = true;
			
		}
		else {
			selectedEntityIndex = 0;
			isEntitySelected_ = false;
		}
	}


	
	void Editor::HandleDeletion() {
		// Check if an entity is selected and if the Backspace key was just pressed
		if (isEntitySelected_ && gameManager_->GetSystems().input_->KeyPressed(Framework::InputSystem::Key::Delete)) {
			const SceneAsset currentScene = SceneManager::GetInstance().GetActiveScene();

			// Tell the SceneManager to remove the entity
			if ((selectedEntityIndex >= 1) && (selectedEntityIndex < static_cast<int>(SceneManager::GetInstance().GetEntitiesInScene(currentScene.name).size() + 1))) {
				Framework::SceneManager::GetInstance().RemoveEntityFromScene(selectedEntityIndex - 1);

				selectedEntityIndex = 0;
				isEntitySelected_ = false;
			}
		}
	}

	#pragma region INTERNAL PANELS

	void Editor::ShowPerformanceProfile() {
		
		ImGui::Begin("Performance Profiler");
		{
			// Add a slider to control how often the profiler updates
			static float updateInterval = 1.0f; // Default to updating once per second
			ImGui::SliderFloat("Update Interval (s)", &updateInterval, 0.1f, 5.0f);
			ImGui::Separator();

			// Update the profiler data only when the timer ends
			profilerUpdateTimer_ += deltatime;
			if (profilerUpdateTimer_ >= updateInterval) {
				profilerUpdateTimer_ = 0.0f;
				profilerDisplayData_ = engine_->GetSystemTimes(); // Get system time
			}

			const double totalFrameTime = engine_->GetTotalFrameTime();
			ImGui::Text("Total Frame Time: %.2f ms (%.1f FPS)", totalFrameTime, ImGui::GetIO().Framerate);
			ImGui::Separator();

			// Display data from all systems using progress bar (can be changed to graph)
			for (const SystemTime& timing : profilerDisplayData_) {
				char label[128];
				sprintf(label, "%.1f%%", timing.percentage);
				ImGui::ProgressBar(timing.percentage / 100.0f, ImVec2(-1.0f, 0.0f), label);
				ImGui::Separator();
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::Text("%s (%.2f ms)", timing.name.c_str(), timing.milliseconds);
			}
		}
		ImGui::End();
	}

	void Editor::ShowLevelEditor() {
		using namespace Framework;
		// --- BUILD IMGUI INTERFACE ---
		// Example of a dockable "Level Editor" window.
		// Create the main dockspace that will fill the entire game window.
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Level Editor");

		const SceneAsset& currentScene = SceneManager::GetInstance().GetActiveScene();
		const std::vector<Entity>& entitiesLoaded = SceneManager::GetInstance().GetEntitiesInScene(currentScene.name);
		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

		// --- Scene Info ---
		if (!currentScene.IsValid()) {
			ImGui::TextDisabled("No Scene Loaded!");
		}
		else {
			ImGui::Text("Scene: %s", currentScene.name.c_str());
			ImGui::Text("Entities: %zu", entitiesLoaded.size());

			// --- Entity Factory ---
		    ImGui::Separator();

			if (ImGui::Button("Create Empty")) {
				Entity newEntity = entityMgr.CreateEntity();
				SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
			}

			ImGui::SameLine();
			if (ImGui::Button("Clone Selected Entity")) {
				Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);
				Entity clonedEntity = entityMgr.CloneEntity(selectedEntity);
				SceneManager::GetInstance().AddEntityToScene(currentScene, clonedEntity);
			}

			// --- Entity Inspector ---
			ImGui::Separator();

			// Button to activate the tile editor mode
			if (ImGui::Button("Open Tilemap Editor")) {
				isTileEditorActive_ = true;
			}

			ImGui::Separator();

			if (entitiesLoaded.empty()) {
				ImGui::TextDisabled("No Entities!");
			}
			else {
				std::vector<std::string> displayNames;
				std::vector<const char*> items;

				displayNames.reserve(entitiesLoaded.size() + 1);
				items.reserve(entitiesLoaded.size() + 1);

				displayNames.push_back("");
				items.push_back(displayNames.back().c_str());

				for (const Entity& entity : entitiesLoaded) {
					std::string name = entityMgr.GetProperties(entity).name;
					displayNames.push_back(name);
					items.push_back(displayNames.back().c_str());
				}

				if (ImGui::Combo("Entities", &selectedEntityIndex, items.data(), static_cast<int>(items.size()), 5)) {
					selectedTagIndex = selectedComponentIndex = 0;
				}

				if ((selectedEntityIndex >= 1) && (selectedEntityIndex < static_cast<int>(entitiesLoaded.size() + 1))) {
					ImGui::Separator();
					isEntitySelected_ = true;

					//ShowPlayerInspector();
				}
				else {
					isEntitySelected_ = false;
				}
			}
		}

		// --- Global Controls ---
		if (ImGui::CollapsingHeader("Global Controls")) {
			static float masterVolume = 0.35f;
			if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
				gameManager_->GetSystems().audio_->setMasterVolume(masterVolume);
			}
			ImGui::Checkbox("Show Grid", &showGrid_);
		}

		// --- section to show the clicked-on entity ---
		ImGui::SeparatorText("World Selection");
		if (isEntitySelected_) {
			ImGui::Text("Selected Entity: [%u]", selectedEntityIndex-1);
			if (ImGui::Button("Deselect")) {
				isEntitySelected_ = false;
				selectedEntityIndex = 0;
			}
			ShowPlayerInspector();
		}
		else {
			ImGui::Text("Selected Entity: None");
			ImGui::TextDisabled("(Click on an object in the game view)");
		}


		// --- Section for Spawning 2500 objects -----//
// --- Section for Spawning 2500 objects -----//
		ImGui::SeparatorText("Spawn 2500 Objects");
		// In your Editor.cpp spawning section:
		if (ImGui::Button("Create 2500 Objects")) {
			Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
			Framework::ResourceManager* rcm = gameManager_->GetSystems().resourceManager_;

			if (ecs) {
				// Create a grid of 2500 entities spread out
				int gridSize = 50; // 50x50 grid = 2500 entities
				float spacing = 0.1f; // Adjust this for proper spacing

				for (int i = 0; i < 2500; ++i) {
					// Calculate grid position
					int row = i / gridSize;
					int col = i % gridSize;

					// Spread entities in a grid formation
					Vector2D position = {
						(col - gridSize / 2) * spacing,  // Center the grid around (0,0)
						(row - gridSize / 2) * spacing
					};

					// FIX: Pass the calculated position, not halfSize
					Entity fake_player = createFakePlayer(*ecs, *rcm, position, "playerSprite_Idle");
					SceneManager::GetInstance().AddEntityToScene(currentScene, fake_player);
				}
				Logger::Get().Info(DebugType::Editor) << "Successfully spawned 2500 objects" << std::endl;
			}
		}
		
		ImGui::End(); // End "Level Editor" window
			
	}

	void Editor::ShowTilemapEditor()
	{
		if (ImGui::Begin("Tile Palette", &isTileEditorActive_)) {
			showGrid_ = true; // Keep grid visible

			ImVec2 buttonSize(64, 64); // Size for image buttons
			ImVec4 bgCol(0, 0, 0, 0); // Transparent background
			ImVec4 tintCol(1, 1, 1, 1); // Normal tint

			// --- Eraser Tool ---
			 // Still use Selectable for the eraser as it doesn't have an image
			if (ImGui::Selectable("Eraser", dragDropPayload_.type == PrefabType::Eraser && isDraggingPrefab_)) {
				// Set payload for eraser drag (optional, can just use click)
				isDraggingPrefab_ = true; // Allow dragging eraser
				dragDropPayload_ = { "Eraser", PrefabType::Eraser, 0 };
			}
			// Or handle eraser purely on click in HandleTileErasing? Current code uses MouseDown.
			ImGui::Separator();

			// --- Prefab Tiles ---
			// Iterate through prepared paletteItems_ which contain texture IDs
			int itemsPerRow = 0;
			const int maxItemsPerRow = 4; // Adjust as needed

			if (paletteItems_.empty() && palettePrepared_) {
				ImGui::TextDisabled("No palette items loaded. Check texture keys and loading logs.");
			}
			for (const auto& item : paletteItems_) {
			//	Framework::Logger::Get().Info(DebugType::Editor) << "Rendering palette button for: " << item.name << ", TextureID: " << item.textureID << std::endl;
				ImGui::PushID(item.name.c_str()); // Unique ID for each button

				// Use ImageButton with the texture ID
				// Need to cast Texture (unsigned int) to ImTextureID (void*)
				ImTextureID textureID = item.textureID; // Correctly cast texture ID

				// CORRECT ORDER: Pass item name as str_id (1st), textureID as tex_ref (2nd)
				if (ImGui::ImageButton(item.name.c_str(), // Use the prefab name as the button ID
					textureID,         // Pass the cast texture ID
					buttonSize,
					item.uv0,
					item.uv1,      
					bgCol,
					tintCol))
				{
					// Click action (optional, could select without starting drag)
					Framework::Logger::Get().Info(DebugType::Editor) << "Clicked palette item: " << item.name << std::endl;
				}

				// --- Drag Source ---
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					// Set payload to be the DraggablePrefabInfo struct itself
					ImGui::SetDragDropPayload("PREFAB_TILE_DND", &item, sizeof(DraggablePrefabInfo));

					// Tooltip/Preview visible while dragging
					ImGui::Text("Placing: %s", item.name.c_str());
					ImGui::Image(textureID, ImVec2(32, 32), item.uv0, item.uv1); // Smaller preview

					// Update editor state that we are dragging
					isDraggingPrefab_ = true;
					dragDropPayload_ = item; // Store the payload info
					Framework::Logger::Get().Info(DebugType::Editor) << "Begin Dragging: " << item.name << std::endl; // Log drag start
					ImGui::EndDragDropSource();

				}
				ImGui::PopID();

				// Arrange buttons in rows
				itemsPerRow++;
				if (itemsPerRow < maxItemsPerRow) {
					ImGui::SameLine();
				}
				else {
					itemsPerRow = 0; // Start new row
				}
			}
			// Ensure cursor is on a new line if the last row wasn't full
			if (itemsPerRow != 0) {
				ImGui::NewLine();
			}

			// --- Display Current Tool (Redundant if preview works) ---
			// if (isDraggingPrefab_ && dragDropPayload_.type != PrefabType::None) {
			//     ImGui::Separator();
			//     ImGui::Text("Dragging: [%s]", dragDropPayload_.name.c_str());
			//     ImGui::Text("Release Left Mouse Button in world to place.");
			// }

		}
		ImGui::End();

		// If window closed, disable tile editor mode
		if (!isTileEditorActive_) {
			showGrid_ = false;
			isDraggingPrefab_ = false; // Stop dragging if window closed
			// Clear payload? Might not be necessary.
			// dragDropPayload_ = {};
		}
	}
	

	void Editor::ShowPlayerInspector() {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);

		std::string& entityName = entityMgr.GetProperties(selectedEntity).name;

		if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
			char nameBuffer[128];
			strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
			nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // ensure null-termination

			if (ImGui::InputText("##Entity Name", nameBuffer, sizeof(nameBuffer))) {
				entityName = nameBuffer; // update ECS data when user edits
			}

			CollisionLayer& entityLayer = entityMgr.GetProperties(selectedEntity).layer;

			// Create a list of layer names for the dropdown
			std::string layerLabels[static_cast<int>(CollisionLayer::Count)]{};
			const char* layerNames[static_cast<int>(CollisionLayer::Count)]{};
			for (int i = 0; i < static_cast<int>(CollisionLayer::Count); ++i) {
				std::string label = std::to_string(i) + ": " + LayerToString(static_cast<CollisionLayer>(i));
				layerLabels[i] = label;
				layerNames[i] = layerLabels[i].c_str();
			}

			int currentLayer = static_cast<int>(entityLayer);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Layer");
			ImGui::SameLine(); // adjust offset spacing
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo("##Layer", &currentLayer, layerNames, IM_ARRAYSIZE(layerNames))) {
				// If a new item was selected, it updates the component's layer
				// with the new value.
				entityLayer = static_cast<CollisionLayer>(currentLayer);
			}

			// --- Component Section ---
			if (ImGui::CollapsingHeader("Components")) {
				ImGui::Text("Attached Components:");
				int compCount = 0;

				for (size_t i = 0; i < componentTypes.size(); ++i) {
					ComponentType type = componentTypes[i];
					if (entityMgr.HasComponent(selectedEntity, type)) {
						if (compCount % 3 != 0) ImGui::SameLine();
						if (selectedComponentIndex > 0 && selectedComponentIndex - 1 == static_cast<int>(i)) {
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
						}
						ImGui::Text("[%s]", componentNames[i + 1].c_str());
						if (selectedComponentIndex > 0 && selectedComponentIndex - 1 == static_cast<int>(i)) {
							ImGui::PopStyleColor();
						}
						++compCount;
					}
				}

				// --- Component Dropdown ---
				ImGui::Combo("Component", &selectedComponentIndex, componentItems.data(), static_cast<int>(componentItems.size()));

				if (selectedComponentIndex > 0) {
					size_t compIndex = static_cast<size_t>(selectedComponentIndex-1); // Offset for "None"

					if (compIndex < componentTypes.size()) {
						ComponentType selectedComponent = componentTypes[compIndex];
						if (entityMgr.HasComponent(selectedEntity, selectedComponent)) {
							ShowComponentSections(selectedEntity, selectedComponent);
						}
						else {
							ImGui::Text("Component Not Added!");
						}

						if (ImGui::Button("Add Component")) {
							if (!entityMgr.HasComponent(selectedEntity, selectedComponent)) {
								entityMgr.AddComponent(selectedEntity, selectedComponent);
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove Component")) {
							if (entityMgr.HasComponent(selectedEntity, selectedComponent)) {
								entityMgr.RemoveComponent(selectedEntity, selectedComponent);
							}
						}
					}
				}
			} // End Components Section

			// --- Tags Section ---
			if (ImGui::CollapsingHeader("Tags"))
			{
				ImGui::Text("Active Tags:");
				int tagCount = 0;

				if (entityMgr.HasComponent(selectedEntity, ComponentType::ActiveTagType)) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
					ImGui::Text("[ActiveTag (Not Removable)]");
					ImGui::PopStyleColor();
					++tagCount;
				}

				for (size_t i = 0; i < tagTypes.size(); ++i) {
					ComponentType type = tagTypes[i];
					if (entityMgr.HasComponent(selectedEntity, type)) {
						if (tagCount % 3 != 0) ImGui::SameLine();
						if (selectedTagIndex > 0 && selectedTagIndex - 1 == static_cast<int>(i)) {
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
						}
						ImGui::Text("[%s]", tagNames[i + 1].c_str());
						if (selectedTagIndex > 0 && selectedTagIndex - 1 == static_cast<int>(i)) {
							ImGui::PopStyleColor();
						}
						++tagCount;
					}
				}

				// --- Tag Dropdown ---
				ImGui::Combo("Tag", &selectedTagIndex, tagItems.data(), static_cast<int>(tagItems.size()));

				if (selectedTagIndex > 0) {
					size_t tagIndex = static_cast<size_t>(selectedTagIndex - 1); // Offset for "None"

					if (tagIndex < tagTypes.size()) {
						ComponentType selectedTag = tagTypes[tagIndex];

						if (ImGui::Button("Add Tag")) {
							if (!entityMgr.HasComponent(selectedEntity, selectedTag)) {
								entityMgr.AddComponent(selectedEntity, selectedTag);
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove Tag")) {
							if (entityMgr.HasComponent(selectedEntity, selectedTag)) {
								entityMgr.RemoveComponent(selectedEntity, selectedTag);
							}
						}
					}
				}
			} // End Tags Section
		}
	}

	void Editor::ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent) {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		ImGui::SeparatorText(ComponentToString(selectedComponent).c_str());
		switch (selectedComponent) {
		case ComponentType::PlayerStatsType: {
			PlayerStats& playerStats = entityMgr.GetComponent<PlayerStats>(selectedEntity);

			ImGui::InputFloat("Move Speed", &playerStats.moveSpeed, 0.1f, 1.f, "%.2f");
			ImGui::InputFloat2("Position", &playerStats.pos.Pos.x, "%.2f");
			break;
		}
		case ComponentType::PositionType: {
			Position& position = entityMgr.GetComponent<Position>(selectedEntity);

			ImGui::InputFloat2("Position", &position.Pos.x, "%.2f");
			break;
		}
		case ComponentType::VelocityType: {
			Velocity& velocity = entityMgr.GetComponent<Velocity>(selectedEntity);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat2("Velocity", &velocity.Vel.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		case ComponentType::AccelerationType: {
			Acceleration& acceleration = entityMgr.GetComponent<Acceleration>(selectedEntity);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat2("Acceleration", &acceleration.Accel.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		case ComponentType::TriangleType: {
			Triangle& triangle = entityMgr.GetComponent<Triangle>(selectedEntity);

			ImGui::InputFloat2("A", &triangle.a.x, "%.2f");
			ImGui::InputFloat2("B", &triangle.b.x, "%.2f");
			ImGui::InputFloat2("C", &triangle.c.x, "%.2f");
			ImGui::ColorEdit4("RGBA", &triangle.color.r);
			break;
		}
		case ComponentType::RigidBodyType: {
			RigidBody& rb = entityMgr.GetComponent<RigidBody>(selectedEntity);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat2("Force", &rb.Force.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);

			float localMass = rb.Mass;
			if (ImGui::InputFloat("Mass", &localMass, 0.1f, 1.0f, "%.2f")) {
				rb.SetMass(localMass);
			}
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Inverse Mass", &rb.InverseMass, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		case ComponentType::AABBColliderType: {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(selectedEntity);

			//ImGui::InputFloat("HalfSize", &col.HalfSize.x, 0.f, 0.f, "%.2f");
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Dim background
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Dim text
			ImGui::InputFloat2("HalfSize (X),(Y)", &col.HalfSize.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2); // Pop both colors
			break;
		}
		case ComponentType::PlayerStateType: {
			PlayerState& pState = entityMgr.GetComponent<PlayerState>(selectedEntity);

			ImGui::Checkbox("Is Grounded?", &pState.isGrounded);
			break;
		}
		case ComponentType::RotationFlagType: {
			RotationFlag& rFlag = entityMgr.GetComponent<RotationFlag>(selectedEntity);

			ImGui::Checkbox("Is Rotating?", &rFlag.isRotating);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Current Rotation", &rFlag.currentRotation, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Target Rotation", &rFlag.targetRotation, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		case ComponentType::TrapPlatformComponentType: {
			TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(selectedEntity);

			ImGui::Checkbox("Is Triggered?", &trapComp.isTriggered);
			ImGui::Checkbox("Has Fallen?", &trapComp.hasFallen);
			ImGui::InputFloat("Trigger Delay", &trapComp.triggerDelay, 0.1f, 1.f, "%.2f");
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Current Delay", &trapComp.currentDelay, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			ImGui::InputFloat("Fall Speed", &trapComp.fallSpeed, 0.1f, 1.f, "%.2f");
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat2("Original Position", &trapComp.originalPosition.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Current Fall Distance", &trapComp.currentFallDistance, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		case ComponentType::ScalingPlatformComponentType: {
			// I'm lazy to do this man
			ImGui::TextDisabled("Properties are Hidden");
			break;
		}
		case ComponentType::TextureComponentType: {
			//TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);

			ImGui::TextDisabled("Properties are Hidden");
			break;
		}
		case ComponentType::AudioComponentType:
		case ComponentType::AnimationComponentType:
		case ComponentType::TextComponentType:
			ImGui::TextDisabled("Properties are Hidden");
			break;
		case ComponentType::SceneOwnedType: {
			SceneOwned& parentScene = entityMgr.GetComponent<SceneOwned>(selectedEntity);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			char buffer[64] = "";
			size_t i = 0;
			for (; i < sizeof(buffer) - 1 && i < parentScene.sceneName.size(); ++i) {
				buffer[i] = parentScene.sceneName[i];
			}
			buffer[i] = '\0';
			ImGui::InputText("Parent Scene", buffer, 64, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			break;
		}
		default:
			break;
		}
		ImGui::Separator();
	}

	void Editor::DrawEditorGrid() {
		
		auto* gfx = graphicsSystem_; // Use stored pointer
		if (!gfx || !cameraSystem_ || baseGridCellSize_ <= 0.0f) return; // Added check for cell size
		Framework::Color gridColor = { 0.5f, 0.5f, 0.5f, 0.3f }; // Semi-transparent grey

		int width = gameManager_->GetSystems().win_->getWidth();
		int height = gameManager_->GetSystems().win_->getHeight();
		float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

		// Get active camera
		Entity activeCamera = cameraSystem_->GetActiveCamera();
		if (activeCamera == 0) return;
		if (activeCamera != 0) {
			cameraSystem_->SetAspectRatio(activeCamera, aspectRatio);
		}

		// Get the current camera view bounds in world space
		float viewLeft, viewRight, viewBottom, viewTop;
		cameraSystem_->GetViewBounds(cameraSystem_->GetActiveCamera(), viewLeft, viewRight, viewBottom, viewTop);

		// Calculate start/end grid lines based on view bounds and the SINGLE world-space cell size
		float startX = std::floor(viewLeft / baseGridCellSize_) * baseGridCellSize_;
		float startY = std::floor(viewBottom / baseGridCellSize_) * baseGridCellSize_;
		float endX = std::ceil(viewRight / baseGridCellSize_) * baseGridCellSize_;
		float endY = std::ceil(viewTop / baseGridCellSize_) * baseGridCellSize_;

		// Determine the world coordinates of the first grid lines just INSIDE or AT the view bounds
		// Align these start points precisely with the grid snapping logic.
		// Find the snapped coordinate of the bottom-left corner, then adjust to find the first line.
		//Vector2D snappedBottomLeft = SnapToGrid({ viewLeft, viewBottom });
		// The first vertical line might be at snappedX - halfCell or snappedX + halfCell.
		// We want the line closest to viewLeft but >= viewLeft.
		// Calculate the index of the snapped cell, find the line coord.
		//int startGridX = static_cast<int>(std::floor(viewLeft / currentGridCellSize_));
		//int startGridY = static_cast<int>(std::floor(viewBottom / currentGridCellSize_));

		// **FIX**: Transform view bounds to screen space for clamping lines
		Vector2D screenViewBottomLeft = cameraSystem_->WorldToScreen({ viewLeft, viewBottom }, activeCamera);
		Vector2D screenViewTopRight = cameraSystem_->WorldToScreen({ viewRight, viewTop }, activeCamera);


		// Draw vertical lines (at positions x) from view bottom to view top
		for (float x = startX; x <= endX + (baseGridCellSize_ * 0.5f); x += baseGridCellSize_) {
			Vector2D worldStart = { x, viewBottom };
			Vector2D worldEnd = { x, viewTop };

			Vector2D screenStart = cameraSystem_->WorldToScreen(worldStart, activeCamera);
			Vector2D screenEnd = cameraSystem_->WorldToScreen(worldEnd, activeCamera);

			gfx->drawLine(screenStart, screenEnd, gridColor);
			
		}

		// Draw horizontal lines (at positions y) from view left to view right
		for (float y = startY; y <= endY + (baseGridCellSize_ * 0.5f); y += baseGridCellSize_) {
			Vector2D worldStart = { viewLeft, y };
			Vector2D worldEnd = { viewRight, y };

			Vector2D screenStart = cameraSystem_->WorldToScreen(worldStart, activeCamera);
			Vector2D screenEnd = cameraSystem_->WorldToScreen(worldEnd, activeCamera);

			gfx->drawLine(screenStart, screenEnd, gridColor);
		}
	}

	Vector2D Editor::SnapToGrid(const Vector2D& worldPos) {
		

		// Calculate index
		int gridX = static_cast<int>(std::floor(worldPos.x  / baseGridCellSize_));
		int gridY = static_cast<int>(std::floor(worldPos.y / baseGridCellSize_));
		// Calculate center
		float snappedX = (static_cast<float>(gridX) * baseGridCellSize_) + (baseGridCellSize_ / 2.0f);
		float snappedY = (static_cast<float>(gridY) * baseGridCellSize_) + (baseGridCellSize_ / 2.0f);
		return { snappedX, snappedY };

	}
	
	void Editor::ShowAssetBrowser(std::vector<std::filesystem::path>& droppedFiles) {
		assetBrowser.RenderUI(droppedFiles);
	}

	#pragma endregion

}