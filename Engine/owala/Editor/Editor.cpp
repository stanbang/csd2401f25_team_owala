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
		case ComponentType::CollisionLayerComponentType:    return "CollisionLayer";
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
	Editor::Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager) : engine_(&engine), gameManager_(&gameManager) {};

	void Editor::SanityCheck() {
		using namespace Framework;

		if (!engine_) {
			throw GameException(DebugType::Editor, "What the sigma, how are you running this without an engine???");
		}

		if (!gameManager_) {
			throw GameException(DebugType::Editor, "GameManager not registered!");
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

		// ---  Calculate Grid Cell Size ---
		// World space is from -1 to 1, so total width/height is 2.0
		gridCellSizeX_ = 2.0f / gridWidth_;
		gridCellSizeY_ = 2.0f / gridHeight_;

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

		initialized = true;
	}

	void Editor::Update(float dt) {
		deltatime = dt;

		// Check if Editor is enabled
		if (!isEnabled) return;

		// Check if Editor has been initialized
		SanityCheck();

		//  --- START THE IMGUI FRAME ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ---  Draw the editor grid ---
		if (showGrid_) {
			DrawEditorGrid();
		}

		// --- Handle Editor Logic ---
		if (isTileEditorActive_) {
			HandleTilePlacement();
			HandleTileErasing();
		}
		else {
			HandleSelection();
		}
		HandleDeletion();

		// --- RENDER PANELS ---

		
		// Render all editor UI panels

		// Level Editor Window
		if (settings.showLevelEditor) {
			ShowLevelEditor();
		}

		// Performance Profiler Window
		if (settings.showDebugInfo) {
			ShowPerformanceProfile();
		}

		
		if (isEntitySelected_) ShowPlayerInspector(); // Show inspector if an entity is selected

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

	Vector2D Editor::GetMouseWorldPosition() {
		auto* input = gameManager_->GetSystems().input_;
		auto* win = gameManager_->GetSystems().win_;
		double mouseX, mouseY;
		input->GetMousePos(&mouseX, &mouseY);

		float ndcX = (static_cast<float>(mouseX) / win->getWidth()) * 2.0f - 1.0f;
		float ndcY = 1.0f - (static_cast<float>(mouseY) / win->getHeight()) * 2.0f; // Y is inverted for NDC

		return { ndcX, ndcY };
	}

	void Editor::HandleTilePlacement() {
		auto* input = gameManager_->GetSystems().input_;

		// Only place if a tile is selected and the left mouse is pressed on the game view
		if (selectedTileType_ == PrefabType::None || selectedTileType_ == PrefabType::Eraser) return;
		if (!input->MouseDown(Framework::InputSystem::MouseButton::Left) || ImGui::GetIO().WantCaptureMouse) return;

		Vector2D snappedPos = SnapToGrid(GetMouseWorldPosition());
		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		// Check if a tile already exists at this position before placing
		for (const std::vector<std::seed_seq::result_type>::iterator::value_type& entity : sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name)) {
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const Vector2D& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				if (std::abs(pos.x - snappedPos.x) < 0.001f && std::abs(pos.y - snappedPos.y) < 0.001f) {
					return; // A tile is already here, so do nothing.
				}
			}
		}

		// Create new entity and add components manually to control size
		Entity newEntity = entityMgr.CreateEntity();
		Vector2D tileHalfSize = { gridCellSizeX_ / 2.0f, gridCellSizeY_ / 2.0f };

		// Common components for all platforms
		entityMgr.AddComponent(newEntity, Framework::Position{ snappedPos });
		entityMgr.AddComponent(newEntity, Framework::Velocity{});
		entityMgr.AddComponent(newEntity, Framework::RigidBody{});
		entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(0.0f); // Static by default
		entityMgr.AddComponent(newEntity, Framework::AABBCollider{ tileHalfSize });
		entityMgr.AddComponent(newEntity, Framework::RotationFlag{});
		entityMgr.AddComponent(newEntity, Framework::CollisionLayerComponent{ Framework::CollisionLayer::Platform });

		const auto* prefab = prefabSystem_.GetPlatformPrefab(selectedTilePrefabName_);
		if (prefab)
			entityMgr.AddComponent(newEntity, Framework::TextureComponent{ prefab->textureID });

		// Type-specific components
		switch (selectedTileType_) {
		case PrefabType::TrapPlatform: {
			const auto* trapPrefab = prefabSystem_.GetTrapPlatformPrefab(selectedTilePrefabName_);
			if (trapPrefab) {
				Framework::TrapPlatformComponent trapComp;
				trapComp.originalPosition = snappedPos;
				trapComp.fallSpeed = trapPrefab->fallSpeed;
				trapComp.triggerDelay = trapPrefab->triggerDelay;
				entityMgr.AddComponent(newEntity, trapComp);
				entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(trapPrefab->mass); // Make it dynamic
			}
			break;
		}
		case PrefabType::ScalingPlatform: {
			const auto* scalePrefab = prefabSystem_.GetScalingPlatformPrefab(selectedTilePrefabName_);
			if (scalePrefab) {
				Framework::ScalingPlatformComponent scaleComp;
				scaleComp.baseHalfSize = tileHalfSize;
				scaleComp.scaleSpeed = scalePrefab->scaleSpeed;
				scaleComp.minScale = scalePrefab->minScale;
				scaleComp.maxScale = scalePrefab->maxScale;
				entityMgr.AddComponent(newEntity, scaleComp);
			}
			break;
		}
		default: // Includes standard Platform
			break;
		}

		if (newEntity != 0) {
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
		}
	}

	void Editor::HandleTileErasing() {
		auto* input = gameManager_->GetSystems().input_;

		if (selectedTileType_ != PrefabType::Eraser) return;
		if (!input->MouseDown(Framework::InputSystem::MouseButton::Left) || ImGui::GetIO().WantCaptureMouse) return;

		Vector2D snappedPos = SnapToGrid(GetMouseWorldPosition());
		Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		Entity entityToErase = 0;

		for (const std::vector<std::seed_seq::result_type>::iterator::value_type& entity : sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name)) {
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const Vector2D& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				if (std::abs(pos.x - snappedPos.x) < 0.001f && std::abs(pos.y - snappedPos.y) < 0.001f) {
					entityToErase = entity;
					break;
				}
			}
		}

		if (entityToErase != 0) {
			sceneMgr.RemoveEntityFromScene(entityToErase);
		}
	}

	void Editor::HandleSpawning() {
		auto* input = gameManager_->GetSystems().input_;

		// Spawn on left click
		if (selectedPrefabType_ != PrefabType::None && input->MousePressed(Framework::InputSystem::MouseButton::Left)) {
			if (!ImGui::GetIO().WantCaptureMouse) {

				Vector2D spawnPos = GetMouseWorldPosition();
				Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
				Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
				Entity newEntity = 0;

				switch (selectedPrefabType_) {
				case PrefabType::Platform:
					newEntity = prefabSystem_.CreatePlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
					break;
				case PrefabType::TrapPlatform:
					newEntity = prefabSystem_.CreateTrapPlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
					break;
				case PrefabType::ScalingPlatform:
					newEntity = prefabSystem_.CreateScalingPlatformFromPrefab(*ecs, selectedPrefabName_, spawnPos);
					break;
				case PrefabType::None: // Should not happen 
					break;
				}

				if (newEntity != 0) {
					sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
				}

				selectedPrefabType_ = PrefabType::None;
				selectedPrefabName_.clear();
			}
		}

		// Cancel on right click
		if (selectedPrefabType_ != PrefabType::None && input->MousePressed(Framework::InputSystem::MouseButton::Right)) {
			selectedPrefabType_ = PrefabType::None;
			selectedPrefabName_.clear();
		}
	}

	
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
		if (isEntitySelected_ && gameManager_->GetSystems().input_->KeyPressed(Framework::InputSystem::Key::Backspace)) {
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
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
				Entity newEntity = entityMgr.CreateEntity();
				SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
			}

			ImGui::SameLine();
			if (ImGui::Button("Clone Selected Entity")) {
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
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
					std::string name = "Entity [" + std::to_string(entity) + "]";
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

		// ---  ENTITY FACTORY / PREFAB SPAWNER ---
		/*ImGui::SeparatorText("Entity Factory");*/

		//// Display a dropdown for each type of prefab we can spawn
		//// uses prefabsystem to collate data of the prefabs
		//if (ImGui::BeginCombo("Platforms", selectedPrefabType_ == PrefabType::Platform ? selectedPrefabName_.c_str() : "Select...")) {
		//	for (const auto& pair : prefabSystem_.GetPlatformPrefabs()) { 
		//		if (ImGui::Selectable(pair.first.c_str())) {
		//			selectedPrefabType_ = PrefabType::Platform;
		//			selectedPrefabName_ = pair.first;
		//		}
		//	}
		//	ImGui::EndCombo();
		//}

		//if (ImGui::BeginCombo("Trap Platforms", selectedPrefabType_ == PrefabType::TrapPlatform ? selectedPrefabName_.c_str() : "Select...")) {
		//	for (const auto& pair : prefabSystem_.GetTrapPlatformPrefabs()) { 
		//		if (ImGui::Selectable(pair.first.c_str())) {
		//			selectedPrefabType_ = PrefabType::TrapPlatform;
		//			selectedPrefabName_ = pair.first;
		//		}
		//	}
		//	ImGui::EndCombo();
		//}

		//if (ImGui::BeginCombo("Scaling Platforms", selectedPrefabType_ == PrefabType::ScalingPlatform ? selectedPrefabName_.c_str() : "Select...")) {
		//	for (const auto& pair : prefabSystem_.GetScalingPlatformPrefabs()) { 
		//		if (ImGui::Selectable(pair.first.c_str())) {
		//			selectedPrefabType_ = PrefabType::ScalingPlatform;
		//			selectedPrefabName_ = pair.first;
		//		}
		//	}
		//	ImGui::EndCombo();
		//}

		//// Display a message to the user if a prefab is selected.
		//if (selectedPrefabType_ != PrefabType::None) {
		//	ImGui::Text("Spawning: [%s]", selectedPrefabName_.c_str());
		//	ImGui::Text("Click in the game view to place. Right-click to cancel.");
		//}


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
		}
		else {
			ImGui::Text("Selected Entity: None");
			ImGui::TextDisabled("(Click on an object in the game view)");
		}


		// --- Section for Spawning 2500 objects -----//
		ImGui::SeparatorText("Spawn 2500 Objects");
		if (ImGui::Button("Create 2500 Objects")) {

			Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
			Framework::ResourceManager* rcm = gameManager_->GetSystems().resourceManager_;
			
			if (ecs) {
				//  Loop 2500 times, creating 2500 player sprites for testing
				for (int i = 0; i < 2500; ++i) {

					Entity fake_player = createFakePlayer(*ecs, *rcm, { 0.01f, 0.01f }, "playerSprite_Idle");
					SceneManager::GetInstance().AddEntityToScene(currentScene, fake_player);
				}
				Logger::Get().Info(DebugType::Editor) << "Successfully spawned 2500 objects" << std::endl;
			}
		}
		
		ImGui::End(); // End "Level Editor" window
			
	}
		
	void Editor::ShowTilemapEditor()
	{
		//  Make this a standalone window. The `&isTileEditorActive_` gives it a close 'X' button.
		ImGui::Begin("Tile Palette", &isTileEditorActive_);

		showGrid_ = true; // Automatically show grid when palette is open

		// Eraser Tool
		if (ImGui::Selectable("Eraser", selectedTileType_ == PrefabType::Eraser)) {
			selectedTileType_ = PrefabType::Eraser;
			selectedTilePrefabName_ = "Eraser";
		}
		ImGui::Separator();

		// Iterate through the different prefab maps to create selectable buttons
		for (const std::list<std::pair<const std::string, Framework::PlatformPrefab>, std::allocator<std::pair<const std::string, Framework::PlatformPrefab>>>::const_iterator::value_type& pair : prefabSystem_.GetPlatformPrefabs()) {
			if (ImGui::Selectable(pair.first.c_str(), selectedTilePrefabName_ == pair.first)) {
				selectedTileType_ = PrefabType::Platform;
				selectedTilePrefabName_ = pair.first;
			}
		}
		for (const std::list<std::pair<const std::string, Framework::PlatformPrefab>, std::allocator<std::pair<const std::string, Framework::PlatformPrefab>>>::const_iterator::value_type& pair : prefabSystem_.GetTrapPlatformPrefabs()) {
			if (ImGui::Selectable(pair.first.c_str(), selectedTilePrefabName_ == pair.first)) {
				selectedTileType_ = PrefabType::TrapPlatform;
				selectedTilePrefabName_ = pair.first;
			}
		}
		for (const std::list<std::pair<const std::string, Framework::PlatformPrefab>, std::allocator<std::pair<const std::string, Framework::PlatformPrefab>>>::const_iterator::value_type& pair : prefabSystem_.GetScalingPlatformPrefabs()) {
			if (ImGui::Selectable(pair.first.c_str(), selectedTilePrefabName_ == pair.first)) {
				selectedTileType_ = PrefabType::ScalingPlatform;
				selectedTilePrefabName_ = pair.first;
			}
		}

		// Display a message to the user if a tile is selected for placement.
		if (selectedTileType_ != PrefabType::None) {
			ImGui::Separator();
			ImGui::Text("Selected: [%s]", selectedTilePrefabName_.c_str());
			ImGui::Text("Left-click in the world to place/erase.");
			if (ImGui::Button("Cancel"))
			{
				selectedTileType_ = PrefabType::None;
				selectedTilePrefabName_.clear();
			}
		}

		ImGui::End();

		// If the window was closed, disable tile editor mode
		if (!isTileEditorActive_) {
			showGrid_ = false;
			selectedTileType_ = PrefabType::None;
			selectedTilePrefabName_.clear();
		}
	}
	

	void Editor::ShowPlayerInspector() {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);

		static std::string entityName; // keep the string alive across frames
		entityName = "Entity [" + std::to_string(selectedEntity) + "]";

		if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Entity: %s", entityName.c_str());

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

			ImGui::InputFloat("HalfSize", &col.HalfSize.x, 0.f, 0.f, "%.2f");
			break;
		}
		case ComponentType::CollisionLayerComponentType: {
			// Checks if the entity has the component
			if (entityMgr.HasComponent(selectedEntity, CollisionLayerComponentType)) {
				auto& layerComp = entityMgr.GetComponent<CollisionLayerComponent>(selectedEntity);

				// Create a list of layer names for the dropdown
				const char* layerNames[static_cast<int>(CollisionLayer::Count)];
				for (int i = 0; i < static_cast<int>(CollisionLayer::Count); ++i) {
					layerNames[i] = LayerToString(static_cast<CollisionLayer>(i));
				}

				int currentLayer = static_cast<int>(layerComp.layer);
				if (ImGui::Combo("Layer", &currentLayer, layerNames, IM_ARRAYSIZE(layerNames))) {
					// If a new item was selected, it updates the component's layer
					// with the new value.
					layerComp.layer = static_cast<CollisionLayer>(currentLayer);
				}
			}
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
		auto* gfx = gameManager_->GetSystems().gfx_;
		if (!gfx) return;

		Color gridColor = { 0.5f, 0.5f, 0.5f, 0.3f };

		// Draw vertical lines
		for (int i = 0; i <= gridWidth_; ++i) {
			float x = -1.0f + i * gridCellSizeX_;
			gfx->drawLine({ x, -1.0f }, { x, 1.0f }, gridColor);
		}

		// Draw horizontal lines
		for (int i = 0; i <= gridHeight_; ++i) {
			float y = -1.0f + i * gridCellSizeY_;
			gfx->drawLine({ -1.0f, y }, { 1.0f, y }, gridColor);
		}
	}

	Vector2D Editor::SnapToGrid(const Vector2D& worldPos) {
		// Calculate the grid cell index the mouse is in
		int gridX = static_cast<int>(floor((worldPos.x + 1.0f) / gridCellSizeX_));
		int gridY = static_cast<int>(floor((worldPos.y + 1.0f) / gridCellSizeY_));

		// Calculate the center of that grid cell
		float snappedX = (gridX * gridCellSizeX_) + (gridCellSizeX_ / 2.0f) - 1.0f;
		float snappedY = (gridY * gridCellSizeY_) + (gridCellSizeY_ / 2.0f) - 1.0f;

		return { snappedX, snappedY };
	}

	#pragma endregion

}