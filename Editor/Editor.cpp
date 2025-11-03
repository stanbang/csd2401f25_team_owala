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
#include "AISystem.h" // Include AISystem for debugging purposes

// Debug
#include "Logger.h"

// Imgui Dependencies
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <random> // for random generation

#include "Fake_Player.h"

// Serialization
#include "EntitySerializer.h"

// Time Management
#include "OwalaTime.h"

// Internal Helpers
namespace {
	using namespace Framework;

	std::string ComponentToString(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PositionType:					return "Position";
		case ComponentType::SpawnPointType:					return "SpawnPoint";
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
		case ComponentType::ButtonType:						return "Button";
		case ComponentType::BirdAIComponentType:			return "BirdAI";
		case ComponentType::SkeletonAIComponentType:		return "SkeletonAI"; 
		case ComponentType::PlayerTagType:					return "PlayerTag";
		case ComponentType::PlatformTagType:				return "PlatformTag";
		case ComponentType::BackgroundTagType:				return "BackgroundTag";
		case ComponentType::EnemyTagType:					return "EnemyTag";
		case ComponentType::SceneOwnedType:					return "SceneOwned";
		case ComponentType::PersistentTagType:				return "PersistentTag";
		case ComponentType::ActiveTagType:					return "ActiveTag";
		case ComponentType::GUITagType:						return "GUITag";
		
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
	// Add these member variable definitions at the top of your Editor class implementation
	// (or wherever you have other member variable initializations)

	// In the constructor, initialize the camera state:
	Editor::Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm)
		: engine_(&engine), gameManager_(&gameManager), resourceManager_(&rcm), assetBrowser(rcm),
		cameraSystem_(gameManager_->GetSystems().camera_),
		graphicsSystem_(gameManager_->GetSystems().gfx_),
		defaultEditorCameraState_{ {0.0f, 0.0f}, 1.0f },
		currentEditorCameraState_{ {0.0f, 0.0f}, 1.0f },
		editorCameraNeedsReset_(false) {
	};

	// Add these implementations to your Editor.cpp file

	void Editor::ResetEditorCamera() {
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			cam.position = defaultEditorCameraState_.position;
			cam.zoom = defaultEditorCameraState_.zoom;
			cameraSystem_->SetZoom(editorCamera_, defaultEditorCameraState_.zoom);

			// Update current state
			currentEditorCameraState_ = defaultEditorCameraState_;
			editorCameraNeedsReset_ = false;

			Framework::Logger::Get().Info(DebugType::Editor)
				<< "Editor camera reset to position ("
				<< cam.position.x << ", " << cam.position.y
				<< ") zoom: " << cam.zoom << std::endl;
		}
	}

	void Editor::SaveEditorCameraState() {
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			currentEditorCameraState_.position = cam.position;
			currentEditorCameraState_.zoom = cam.zoom;

			Framework::Logger::Get().Info(DebugType::Editor)
				<< "Saved editor camera state: position ("
				<< currentEditorCameraState_.position.x << ", " << currentEditorCameraState_.position.y
				<< ") zoom: " << currentEditorCameraState_.zoom << std::endl;
		}
	}

	void Editor::RestoreEditorCameraState() {
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			cam.position = currentEditorCameraState_.position;
			cam.zoom = currentEditorCameraState_.zoom;
			cameraSystem_->SetZoom(editorCamera_, currentEditorCameraState_.zoom);

			Framework::Logger::Get().Info(DebugType::Editor)
				<< "Restored editor camera state: position ("
				<< cam.position.x << ", " << cam.position.y
				<< ") zoom: " << cam.zoom << std::endl;
		}
	}

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
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled, tweak WindowRounding/WindowBg so platform windows look identical
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
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
		fs::path rootPath = fs::absolute(exePath / ""); // Moves out of Debug/Release, then out of build, to reach Engine root
		assetBrowser.Initialize(rootPath);

		// Prepare Dropdown arrays for Entity Inspector
		componentNames.clear();
		componentItems.clear();
		componentTypes.clear();
		componentTypes.clear();
		tagNames.clear();
		tagItems.clear();
		tagTypes.clear();

		componentNames.push_back("");
		tagNames.push_back("");

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

		for (int i = 0; i < static_cast<int>(ComponentType::MAX_COMPONENTS); ++i) {
			ComponentType type = static_cast<ComponentType>(i);
			if (!entityMgr.IsTagComponent(type)) {
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
		if (!prefabSystem_.LoadPrefabsFromFile("Data/platformPrefab.json")) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to load prefabs from Data/platformPrefab.json" << std::endl;
		}

		// --- Prepare Palette Items (Load Textures) ---
		// (assuming this stays as-is)
		firstUpdate = true;


		cameraSystem_ = gameManager_->GetSystems().camera_;
		graphicsSystem_ = gameManager_->GetSystems().gfx_;

		if (cameraSystem_ && editorCamera_ == 0) {
			editorCamera_ = cameraSystem_->CreateCamera({ 0.0f, 0.0f }, 0, false); // not active for game
			cameraSystem_->SetZoom(editorCamera_, editorZoom_);
			float aspect = gameViewportSize_.x / gameViewportSize_.y;
			cameraSystem_->SetAspectRatio(editorCamera_, aspect);
		}

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
	void Editor::Update() {

		if (!palettePrepared_ && resourceManager_) {
			Framework::Logger::Get().Info(DebugType::Editor) << "Running PreparePaletteItems..." << std::endl;
			PreparePaletteItems();
			palettePrepared_ = true;
			Framework::Logger::Get().Info(DebugType::Editor) << "PreparePaletteItems finished. paletteItems_ size: " << paletteItems_.size() << std::endl;
		}

		// Determine if ImGui currently "owns" the input
		//bool imguiHasFocus = io.WantCaptureKeyboard || io.WantCaptureMouse;

		// Tell the InputSystem whether to lock gameplay input
		Framework::InputSystem* input = gameManager_->GetSystems().input_;
		input->SetCursorLocked(false);

		if (!isEnabled) return;
		SanityCheck();

		// --- START IMGUI FRAME ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// --- DOCKSPACE SETUP ---
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Owala Editor Dockspace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

		if (firstUpdate)
		{
			firstUpdate = false;
			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->Size);

			ImGuiID dock_main_id = dockspaceID;

			ImGuiID dock_toolbar = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.03f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Toolbar", dock_toolbar);

			if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_toolbar)) {
				node->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar | ImGuiDockNodeFlags_NoDocking | ImGuiDockNodeFlags_NoTabBar;
			}

			ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Inspector", dock_right);
			ImGui::DockBuilderDockWindow("Asset Editor", dock_right);
			ImGui::DockBuilderDockWindow("Tile Palette", dock_right);

			ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.35f, nullptr, &dock_right);
			ImGui::DockBuilderDockWindow("Performance Profiler", dock_right_bottom);

			ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.35f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Asset Viewer", dock_bottom);
			ImGuiID dock_bottom_left = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.2f, nullptr, &dock_bottom);
			ImGui::DockBuilderDockWindow("Project Explorer", dock_bottom_left);

			ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Level Editor", dock_left);
			ImGui::DockBuilderDockWindow("Hierarchy", dock_left);

			ImGui::DockBuilderDockWindow("Game", dock_main_id);
			ImGui::DockBuilderFinish(dockspaceID);
		}

		// --- UPDATE VIEWPORT DATA ---
		UpdateViewportData();

		gameManager_->GetSystems().input_->SetCursorLocked(false);

		// === EDITOR CAMERA CONTROLS ===
		//{
		//	Framework::InputSystem* input = gameManager_->GetSystems().input_;

		//	if (editorCamera_ != 0) {
		//		Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
		//		
		//		// Only allow editor camera input when ImGui is focused
		//		if (imguiHasFocus)
		//		{
		//			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);

		//			if (input->KeyDown(Framework::InputSystem::Key::W)) cam.position.y += dt * 2.0f;
		//			if (input->KeyDown(Framework::InputSystem::Key::S)) cam.position.y -= dt * 2.0f;
		//			if (input->KeyDown(Framework::InputSystem::Key::A)) cam.position.x -= dt * 2.0f;
		//			if (input->KeyDown(Framework::InputSystem::Key::D)) cam.position.x += dt * 2.0f;

		//			if (input->KeyPressed(Framework::InputSystem::Key::Z)) cam.zoom *= 0.8f;
		//			if (input->KeyPressed(Framework::InputSystem::Key::X)) cam.zoom *= 1.25f;
		//		}

		//	}
		//}
		// === END EDITOR CAMERA CONTROLS ===

		const float dt = Framework::Time::UnscaledDeltaTime();

		if (editorCamera_ != 0)
		{
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);

			// Move camera with WASD
			if (input->KeyDown(Framework::InputSystem::Key::W)) cam.position.y += 5.0f * dt;
			if (input->KeyDown(Framework::InputSystem::Key::S)) cam.position.y -= 5.0f * dt;
			if (input->KeyDown(Framework::InputSystem::Key::A)) cam.position.x -= 5.0f * dt;
			if (input->KeyDown(Framework::InputSystem::Key::D)) cam.position.x += 5.0f * dt;

			// Zoom in/out using Up / Down arrow keys
			if (input->KeyDown(Framework::InputSystem::Key::Up)) {
				cam.zoom *= 0.98f; // zoom in
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}
			if (input->KeyDown(Framework::InputSystem::Key::Down)) {
				cam.zoom *= 1.02f; // zoom out
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}

			// Optional: reset zoom with R
			if (input->KeyPressed(Framework::InputSystem::Key::R)) {
				cam.zoom = 1.0f;
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}
		}


		// --- Handle Editor Logic ---
		HandleSpawning();
		HandleSelection();
		HandleDeletion();

		if (isTileEditorActive_ && isDraggingPrefab_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			HandleDragDropPlacement();
		}
		if (isTileEditorActive_ && dragDropPayload_.type == PrefabType::Eraser) {
			HandleTileErasing();
		}
		if (!isDraggingPrefab_ && dragDropPayload_.type != PrefabType::Eraser) {
			HandleSelection();
		}
		HandleDeletion();

		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			isDraggingPrefab_ = false;
		}

		// --- RENDER PANELS ---
		if (settings.showToolbar) ShowToolbar();
		if (settings.showInspector) ShowInspector();
		if (settings.showDebugInfo) ShowPerformanceProfile();
		if (settings.showAssetBrowser) ShowAssetBrowser(droppedFilePaths_);
		if (settings.showHierarchy) ShowHierarchy();
		if (settings.showLevelEditor) ShowLevelEditor();
		if (isTileEditorActive_) ShowTilemapEditor();
		if (settings.showGameViewport) ShowGameViewport();
		ShowAIDebugPanel();
		ImGui::End();

		// --- END IMGUI FRAME ---
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
#pragma region INTERNAL HELPERS

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
		//for (const auto& pair : prefabSystem_.GetTrapPlatformPrefabs()) {
		//	AddPaletteItem(pair.first, PrefabType::TrapPlatform, pair.second.textureID);
		//}
		//for (const auto& pair : prefabSystem_.GetScalingPlatformPrefabs()) {
		//	AddPaletteItem(pair.first, PrefabType::ScalingPlatform, pair.second.textureID);
		//}
		Framework::Logger::Get().Info(DebugType::Editor) << "Prepared " << paletteItems_.size() << " palette items." << std::endl;
	}

	Vector2D Editor::GetMouseWorldPosition() {
		if (!cameraSystem_ || editorCamera_ == 0)
			return { 0.0f, 0.0f };

		ImVec2 mouse = ImGui::GetMousePos();

		// convert to relative viewport coords
		float relX = mouse.x - gameViewportPos_.x;
		float relY = mouse.y - gameViewportPos_.y;

		if (relX < 0 || relY < 0 ||
			relX > gameViewportSize_.x || relY > gameViewportSize_.y)
			return { 0.0f, 0.0f };

		// NDC conversion (Y flipped)
		float ndcX = (relX / gameViewportSize_.x) * 2.0f - 1.0f;
		float ndcY = 1.0f - (relY / gameViewportSize_.y) * 2.0f; // this flips Y!

		const Camera& cam = cameraSystem_->GetCamera(editorCamera_);
		Vector2D camPos = cam.position;
		Vector2D camSize = cam.size;

		// Map NDC → world
		float worldX = camPos.x + ndcX * (camSize.x * 0.5f);
		float worldY = camPos.y + ndcY * (camSize.y * 0.5f);
		return { worldX, worldY };
	}


	void Editor::HandleDragDropPlacement()
	{
		if (!isDraggingPrefab_ || !isMouseOverGameViewport_) {
			isDraggingPrefab_ = false;
			return;
		}
		if (!cameraSystem_ || editorCamera_ == 0)
			return;

		const Camera& cam = cameraSystem_->GetCamera(editorCamera_);

		// --- MOUSE POSITION (ImGui → World) ---
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 viewportPos = gameViewportPos_;
		ImVec2 viewportSize = gameViewportSize_;

		// Convert from ImGui → NDC → World
		float ndcX = ((mousePos.x - viewportPos.x) / viewportSize.x) * 2.0f - 1.0f;
		float ndcY = (1.0f - ((mousePos.y - viewportPos.y) / viewportSize.y) * 2.0f);

		Vector2D worldMousePos = {
			cam.position.x + ndcX * (cam.size.x * 0.5f),
			cam.position.y + ndcY * (cam.size.y * 0.5f)
		};

		// --- SNAP TO GRID ---
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		// Prevent duplicates
		const float tolerance = baseGridCellSize_ * 0.1f;
		for (const auto& entity : sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name)) {
			if (!entityMgr.HasComponent(entity, Framework::PlatformTagType)) continue;
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const auto& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				if (fabs(pos.x - snappedPos.x) < tolerance && fabs(pos.y - snappedPos.y) < tolerance) {
					isDraggingPrefab_ = false;
					return;
				}
			}
		}

		// --- ENTITY CREATION ---
		Entity newEntity = entityMgr.CreateEntity();

		// Maintain square tiles visually across aspect ratios
		float aspect = gameViewportSize_.x / gameViewportSize_.y;
	Vector2D tileHalfSize = { baseGridCellSize_ * 0.5f, baseGridCellSize_ * 0.5f };
		entityMgr.AddComponent(newEntity, Framework::PlatformTag{});
		entityMgr.AddComponent(newEntity, Framework::Position{ snappedPos });
		entityMgr.AddComponent(newEntity, Framework::Velocity{});
		entityMgr.AddComponent(newEntity, Framework::RigidBody{});
		entityMgr.GetComponent<Framework::RigidBody>(newEntity).SetMass(0.0f);
		entityMgr.AddComponent(newEntity, Framework::AABBCollider{ tileHalfSize });
		entityMgr.AddComponent(newEntity, Framework::RotationFlag{});
		entityMgr.GetProperties(newEntity) = { "platforms", CollisionLayer::Platform };

		std::string textureKey;
		const auto* prefab = prefabSystem_.GetPlatformPrefab(dragDropPayload_.name);
		if (prefab)
			textureKey = prefab->textureID;

		if (!textureKey.empty()) {
			entityMgr.AddComponent(newEntity, Framework::TextureComponent{ textureKey, tileHalfSize });
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
		}

		isDraggingPrefab_ = false;
	}


	void Editor::DrawDragDropPreview(ImDrawList* drawList)
	{
		if (!isDraggingPrefab_ ||
			dragDropPayload_.type == PrefabType::None ||
			dragDropPayload_.type == PrefabType::Eraser)
			return;

		if (!cameraSystem_ || editorCamera_ == 0)
			return;

		Vector2D worldMousePos = GetMouseWorldPosition();
		dragDropPreviewPos_ = SnapToGrid(worldMousePos);

		Texture previewTextureID = dragDropPayload_.textureID;
		if (previewTextureID == 0)
			return;

		// --- FIX: Use same world size as placed tiles ---
		Vector2D previewHalfSize = { baseGridCellSize_ * 0.5f, baseGridCellSize_ * 0.5f };

		Vector2D worldTopLeft = {
			dragDropPreviewPos_.x - previewHalfSize.x,
			dragDropPreviewPos_.y + previewHalfSize.y
		};
		Vector2D worldBottomRight = {
			dragDropPreviewPos_.x + previewHalfSize.x,
			dragDropPreviewPos_.y - previewHalfSize.y
		};

		ImVec2 screenTopLeft = WorldToImGuiScreen(worldTopLeft);
		ImVec2 screenBottomRight = WorldToImGuiScreen(worldBottomRight);

		ImVec2 uv0 = dragDropPayload_.uv0;
		ImVec2 uv1 = dragDropPayload_.uv1;
		ImU32 tintColor = IM_COL32(255, 255, 255, 150);

		ImTextureID texID = (ImTextureID)(intptr_t)previewTextureID;
		drawList->AddImage(texID, screenTopLeft, screenBottomRight, uv0, uv1, tintColor);
	}






	void Editor::HandleTileErasing() {
		auto* input = gameManager_->GetSystems().input_;

		if (dragDropPayload_.type != PrefabType::Eraser) return; // Check payload type now
		// Use MouseDown for continuous erasing
		// Check if mouse is over game viewport
		if (!input->MouseDown(Framework::InputSystem::MouseButton::Left) || !isMouseOverGameViewport_) {
			return;
		}

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
			entityMgr.DestroyEntity(entityToErase);
			selectedEntityIndex = 0;
			isEntitySelected_ = false;
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

				/*	switch (selectedPrefabType_) {
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
					}*/

				newEntity = prefabSystem_.CreateEntityFromPrefab(*ecs, selectedPrefabName_, spawnPos);
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

		// Check if click is over game viewport
		if (!input->MousePressed(Framework::InputSystem::MouseButton::Left) ||
			!isMouseOverGameViewport_) {
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
			selectedEntityIndex = bestCandidate + 1;
			isEntitySelected_ = true;
			selectedComponentIndex = 0;
			selectedTagIndex = 0;

		}
		else {
			selectedEntityIndex = 0;
			isEntitySelected_ = false;
			selectedComponentIndex = 0;
			selectedTagIndex = 0;
		}
	}

	void Editor::HandleDeletion() {
		// Check if an entity is selected and if the Backspace key was just pressed
		if (isEntitySelected_ && gameManager_->GetSystems().input_->KeyPressed(Framework::InputSystem::Key::Delete)) {
			const SceneAsset currentScene = SceneManager::GetInstance().GetActiveScene();
			EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

			// Tell the SceneManager to remove the entity
			if ((selectedEntityIndex >= 1) && (selectedEntityIndex < static_cast<int>(SceneManager::GetInstance().GetEntitiesInScene(currentScene.name).size() + 1))) {
				entityMgr.DestroyEntity(selectedEntityIndex - 1);

				selectedEntityIndex = 0;
				isEntitySelected_ = false;
			}
		}
	}

#pragma region COPY AND PASTING APIs

	void Editor::CopyEntityToClipboard(const Entity& entity) {
		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		rapidjson::Value entityData(rapidjson::kObjectType);
		EntitySerializer::SerializeEntity(entity, *ecs, entityData, alloc);
		doc.CopyFrom(entityData, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
		entityClipboard = buffer.GetString();
	}

	void Editor::PasteEntityFromClipboard() {
		// Check if there is an entity in the clipboard
		if (!entityClipboard.has_value()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "No entity in clipboard to paste!" << std::endl;
			return;
		}

		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		rapidjson::Document doc;

		doc.Parse(entityClipboard->c_str());
		if (doc.HasParseError()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to parse entity data from clipboard!" << std::endl;
			return;
		}

		Entity pastedEntity = EntitySerializer::DeserializeEntity(*ecs, doc);
		Framework::SceneManager::GetInstance().AddEntityToScene(Framework::SceneManager::GetInstance().GetActiveScene(), pastedEntity);
	}

#pragma endregion

#pragma endregion

	// --- Panel Rendering ---
#pragma region INTERNAL PANELS

#pragma region PERFORMANCE PROFILER PANEL
	void Editor::ShowPerformanceProfile() {

		ImGui::Begin("Performance Profiler");
		{
			// Add a slider to control how often the profiler updates
			static float updateInterval = 1.0f; // Default to updating once per second
			ImGui::SliderFloat("Update Interval (s)", &updateInterval, 0.1f, 5.0f);
			ImGui::Separator();

			// Update the profiler data only when the timer ends
			profilerUpdateTimer_ += Framework::Time::UnscaledDeltaTime();
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

#pragma endregion

#pragma region LEVEL EDITOR PANEL

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

			SaveAndLoadSection();

			// --- Entity Factory ---
			ImGui::Separator();

			if (ImGui::Button("Create Empty")) {
				Entity newEntity = entityMgr.CreateEntity();
				SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
			}

			ImGui::SameLine();
			if (ImGui::Button("Clone Selected Entity") && selectedEntityIndex > 0) {
				Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);
				Entity clonedEntity = entityMgr.CloneEntity(selectedEntity);
				SceneManager::GetInstance().AddEntityToScene(currentScene, clonedEntity);
			}

			// --- Entity Inspector ---
			ImGui::Separator();

			// Add reset camera button
			if (ImGui::Button("Reset Editor Camera")) {
				ResetEditorCamera();
			}
			ImGui::SameLine();

			// Button to activate the tile editor mode
			if (ImGui::Button("Open Tilemap Editor")) {
				isTileEditorActive_ = true;
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
			ImGui::Text("Selected Entity: [%u]", selectedEntityIndex - 1);
			if (ImGui::Button("Deselect")) {
				isEntitySelected_ = false;
				selectedEntityIndex = 0;
			}
		}
		else {
			ImGui::Text("Selected Entity: None");
			ImGui::TextDisabled("(Click on an object in the game view)");
		}


		// --- Section for Spawning 1000 objects -----//
		ImGui::SeparatorText("Spawn 1000 Objects");
		// In your Editor.cpp spawning section:
		if (ImGui::Button("Create 1000 Objects")) {
			Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
			Framework::ResourceManager* rcm = gameManager_->GetSystems().resourceManager_;

			if (ecs) {
				// Create a grid of 2500 entities spread out
				int gridSize = 50; // 50x50 grid = 2500 entities
				float spacing = 0.1f; // Adjust this for proper spacing

				for (int i = 0; i < 1000; ++i) {
					// Calculate random position from -1.0 to 1.0
					float x = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
					float y = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
					Vector2D position = { x, y };

					// Pass the calculated position, not halfSize
					Entity fake_player = createFakePlayer(*ecs, *rcm, position, "Eli_Idle_Spritesheet.png");
					SceneManager::GetInstance().AddEntityToScene(currentScene, fake_player);
				}
				Logger::Get().Info(DebugType::Editor) << "Successfully spawned 1000 objects" << std::endl;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Spawn Test Bird")) {
			// Get the systems we need
			SceneManager& sceneMgr = SceneManager::GetInstance();
			const SceneAsset& currScene = sceneMgr.GetActiveScene();

			//  Create entity
			Entity bird = entityMgr.CreateEntity();

			//  Set properties
			entityMgr.GetProperties(bird) = { "Test Bird", CollisionLayer::Enemy };

			// Add components required for the AI
			entityMgr.AddComponent<Position>(bird, { {0.0f, 0.5f} }); // Spawn at (0, 0.5)
			entityMgr.AddComponent<Velocity>(bird, {});              // Start with no velocity
			entityMgr.AddComponent<EnemyTag>(bird, {});            // Tag for the AISystem
			entityMgr.AddComponent<BirdAIComponent>(bird, {});      // Add the AI brain
			entityMgr.AddComponent<AnimationComponent>(bird, {});

			RigidBody rb;
			rb.SetMass(0.0f); // Give it some mass
			rb.isAffectedByGravity = false; // Make sure it doesn't fall
			entityMgr.AddComponent<RigidBody>(bird, rb);

			//  Add components to make it visible and selectable
			Vector2D texHalfSize = { 0.1f, 0.2f }; // Set a default size
			Vector2D aabbHalfSize = { 0.01f, 0.01f }; // Set a default size

			// texture is loaded in TestScene.cpp
			//entityMgr.AddComponent<TextureComponent>(bird, { "Eli_Idle_Spritesheet.png", halfSize });
			entityMgr.AddComponent<TextureComponent>(bird, { "Bird_Enemy_SleepIdle_Spritesheet.png", texHalfSize });
			
		    entityMgr.AddComponent<AABBCollider>(bird, { aabbHalfSize });

			// Add it to the active scene
			sceneMgr.AddEntityToScene(currScene, bird);

			Logger::Get().Info(DebugType::Editor) << "Spawned Test Bird (Entity ID: "
				<< bird << ")";
		}

		ImGui::SameLine();
		if (ImGui::Button("Spawn Test Skeleton")) {
			// Get the systems we need
			EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
			SceneManager& sceneMgr = SceneManager::GetInstance();
			const SceneAsset& currentScene = sceneMgr.GetActiveScene();

			// Create entity
			Entity skeleton = entityMgr.CreateEntity();

			// Set properties
			entityMgr.GetProperties(skeleton) = { "Test Skeleton", CollisionLayer::Enemy };

			// Add components required for the AI
			entityMgr.AddComponent<Position>(skeleton, { {-0.5f, 0.0f} }); // Spawn at (0, 0.5)
			entityMgr.AddComponent<Velocity>(skeleton, {});              // Start with no velocity
			entityMgr.AddComponent<EnemyTag>(skeleton, {});            // Tag for the AISystem
			entityMgr.AddComponent<SkeletonAIComponent>(skeleton, {});  // Add the Skeleton AI brain
			entityMgr.AddComponent<PlayerState>(skeleton, {});      // Needed for grounded/facing checks
			

			// Add physics, but make it affected by gravity
			RigidBody rb;
			rb.SetMass(1.0f); // Give it some mass
			rb.isAffectedByGravity = true; // Make it fall
			entityMgr.AddComponent<RigidBody>(skeleton, rb);

			// Add components to make it visible and selectable
			Vector2D halfSize = { 0.1f, 0.1f }; // Set a default size
			entityMgr.AddComponent<TextureComponent>(skeleton, { "Eli_Idle_Spritesheet.png", halfSize });
			entityMgr.AddComponent<AABBCollider>(skeleton, { halfSize });

			// Add it to the active scene
			sceneMgr.AddEntityToScene(currentScene, skeleton);

			Logger::Get().Info(DebugType::Editor) << "Spawned Test Skeleton (Entity ID: "
				<< skeleton << ")";
		}
		

		ImGui::End(); // End "Level Editor" window
	}

	void Editor::SaveAndLoadSection() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		const SceneAsset& currentScene = sceneMgr.GetActiveScene();
		const std::vector<Entity>& entitiesLoaded = sceneMgr.GetEntitiesInScene(currentScene.name);

		// --- Save (Overwrite/Quick Save. No changes to scene name or file path) ---
		if (ImGui::Button("Save")) {
			// Reset entity states & stop game on save 
			isGamePlaying = false;
			gameManager_->GetSystems().levelResetter_->RunReset();
			std::string filePath = FileIOKeys::SCENES_FILEPATH_PREFIX; filePath += FileIOSystem::SanitizeFileName(currentScene.name) + ".json";

			// If its a newly created scene, create new filepath for it based on scene name.
			if (currentScene.path.empty()) {
				sceneMgr.UpdateScenePath(currentScene.sceneIndex, filePath);
			}

			Logger::Get().Info(DebugType::Editor) << "Saving scene: " << currentScene.name << " to: " << filePath << std::endl;

			// Serialize Scene name and entities to its own json file
			EntitySerializer::SerializeSceneData(entitiesLoaded, *(gameManager_->GetSystems().ecs_), filePath, currentScene.name);
			// Save scene asset to json file that stores all scene assets
			FileIOSystem::SaveSceneAsset(currentScene);
		}

		// --- Create New Scene --- 
		ImGui::SameLine();
		if (ImGui::Button("New Scene")) {
			ImGui::OpenPopup("Create New Scene");
		}

		// Create New Scene Modal
		if (ImGui::BeginPopupModal("Create New Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// Text Input for scene name
			static char sceneNameBuffer[256] = "";
			ImGui::Text("Scene Name:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputText("##SceneName", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));
			static bool duplicateNameError = false, emptySceneName = false;
			// Prevent game input system from capturing mouse/keyboard input
			gameManager_->GetSystems().input_->SetKeyboardLocked(true);

			if (duplicateNameError) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Scene name already in use!");
			} 
			if (emptySceneName) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Empty scene name!");
			}

			if (ImGui::Button("Confirm", ImVec2(120, 0)))
			{
				// Check empty scene name and duplicate scene name
				std::string sceneName = sceneNameBuffer;
				emptySceneName = sceneName.empty();
				duplicateNameError = sceneMgr.CheckSceneNameExists(sceneName);

				if (!duplicateNameError && !emptySceneName) {
					sceneMgr.CreateSceneAsset(sceneName, "", true);
					ImGui::CloseCurrentPopup();
					gameManager_->GetSystems().input_->SetKeyboardLocked(false);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
				gameManager_->GetSystems().input_->SetKeyboardLocked(false);
				// Clear buffer and reset error variables
				sceneNameBuffer[0] = '\0';
				duplicateNameError = false;
				emptySceneName = false;
			}

			ImGui::EndPopup();
		}

		// Display list of scenes available to load
		if (ImGui::CollapsingHeader("Scenes List")) {
			for (const auto& [id, scene] : sceneMgr.GetAllSceneAssets()) {
				bool isCurrentScene = (id == sceneMgr.GetActiveScene().sceneIndex);

				if (isCurrentScene) {
					ImGui::BeginDisabled();
				}

				if (ImGui::Button(scene.name.c_str())) {
					Logger::Get().Info(DebugType::Editor) << "Attempting to load scene: " << scene.name << std::endl;
					if (!isCurrentScene) {
						isGamePlaying = false;
						gameManager_->GetSystems().levelResetter_->RunReset();
						sceneMgr.LoadScene(id);
					}
				}
				
				if (isCurrentScene) {
					ImGui::EndDisabled();
				}
			}
		}
	}

#pragma endregion

#pragma region TILEMAP EDITOR PANEL

	void Editor::ShowTilemapEditor()
	{
		static bool wasTileEditorOpen = false; // Tracks open/close transition

		if (ImGui::Begin("Tile Palette", &isTileEditorActive_)) {
			// --- ENTER TILE EDITOR (open event) ---
			if (!wasTileEditorOpen) {
				wasTileEditorOpen = true;

				if (engine_ && !engine_->IsEditorMode()) {
					// Save current state before switching
					SaveEditorCameraState();

					engine_->SetMode(Framework::EngineMode::Editor);

					// Reset editor camera to known state when entering tile editor
					ResetEditorCamera();

					Framework::Logger::Get().Info(DebugType::Editor)
						<< "[Editor] Switched to EDITOR mode (Tilemap open)" << std::endl;
				}
			}

			showGrid_ = true; // Keep grid visible

			ImVec2 buttonSize(64, 64);
			ImVec4 bgCol(0, 0, 0, 0);
			ImVec4 tintCol(1, 1, 1, 1);

			// --- Eraser Tool ---
			if (ImGui::Selectable("Eraser", dragDropPayload_.type == PrefabType::Eraser && isDraggingPrefab_)) {
				isDraggingPrefab_ = true;
				dragDropPayload_ = { "Eraser", PrefabType::Eraser, 0 };
			}
			ImGui::Separator();

			// --- Prefab Tiles ---
			int itemsPerRow = 0;
			const int maxItemsPerRow = 4;

			if (paletteItems_.empty() && palettePrepared_) {
				ImGui::TextDisabled("No palette items loaded. Check texture keys and loading logs.");
			}

			for (const auto& item : paletteItems_) {
				ImGui::PushID(item.name.c_str());
				ImTextureID textureID = item.textureID;

				if (ImGui::ImageButton(item.name.c_str(),
					textureID,
					buttonSize,
					item.uv0,
					item.uv1,
					bgCol,
					tintCol))
				{
					Framework::Logger::Get().Info(DebugType::Editor)
						<< "Clicked palette item: " << item.name << std::endl;
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", item.name.c_str());

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					ImGui::SetDragDropPayload("PREFAB_TILE_DND", &item, sizeof(DraggablePrefabInfo));
					ImGui::Text("Placing: %s", item.name.c_str());
					ImGui::Image(textureID, ImVec2(32, 32), item.uv0, item.uv1);

					isDraggingPrefab_ = true;
					dragDropPayload_ = item;

					Framework::Logger::Get().Info(DebugType::Editor)
						<< "Begin Dragging: " << item.name << std::endl;

					ImGui::EndDragDropSource();
				}

				ImGui::PopID();

				itemsPerRow++;
				if (itemsPerRow < maxItemsPerRow)
					ImGui::SameLine();
				else
					itemsPerRow = 0;
			}

			if (itemsPerRow != 0)
				ImGui::NewLine();
		}
		ImGui::End();

		// --- EXIT TILE EDITOR (close event) ---
		if (!isTileEditorActive_ && wasTileEditorOpen) {
			wasTileEditorOpen = false;

			if (engine_ && engine_->IsEditorMode()) {
				// Save current editor camera state before switching back
				SaveEditorCameraState();

				engine_->SetMode(Framework::EngineMode::Play);
				Framework::Logger::Get().Info(DebugType::Editor)
					<< "[Editor] Switched back to PLAY mode (Tilemap closed)" << std::endl;
			}

			showGrid_ = false;
			isDraggingPrefab_ = false;
		}
	}



#pragma endregion

#pragma region HIERARCHY PANEL

	void Editor::ShowHierarchy() {
		using namespace Framework;

		//ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 1500, 20), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowSize(ImVec2(380, ImGui::GetIO().DisplaySize.y - 300), ImGuiCond_FirstUseEver);

		ImGui::Begin("Hierarchy");

		const SceneAsset& currentScene = SceneManager::GetInstance().GetActiveScene();

		// --- Scene Info ---
		if (!currentScene.IsValid()) {
			ImGui::TextDisabled("No Scene Loaded!");
			ImGui::End();
			return;
		}

		const std::vector<Entity>& entitiesLoaded = SceneManager::GetInstance().GetEntitiesInScene(currentScene.name);
		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		if (ImGui::BeginChild("HierarchyList", ImVec2{ 0, 0 }, true)) {
			int localWidgetID = 0;

			// Clipper for large list optimization(This only process entities in view in the list)
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(entitiesLoaded.size()));

			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
					const Entity entity = entitiesLoaded[i];

					std::string name = entityMgr.GetProperties(entity).name;
					ImGui::PushID(localWidgetID++);
					bool selected = (static_cast<unsigned int>((selectedEntityIndex - 1)) == entity);
					if (ImGui::Selectable(name.c_str(), &selected)) {
						selectedTagIndex = selectedComponentIndex = 0;
						selectedEntityIndex = entity + 1;
						isEntitySelected_ = true;
					}
					ImGui::PopID();

					// Allows right-click selection
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						selectedTagIndex = selectedComponentIndex = 0;
						selectedEntityIndex = entity + 1;
						isEntitySelected_ = true;
					}

					// Selectable Context Menu
					if (ImGui::BeginPopupContextItem()) {
						// Copy
						if (ImGui::MenuItem("Copy")) {
							CopyEntityToClipboard(entity);
						}
						// Paste
						if (entityClipboard) {
							if (ImGui::MenuItem("Paste")) {
								PasteEntityFromClipboard();
							}
						}
						else {
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
							ImGui::Text("Paste");
							ImGui::PopStyleColor(1);
						}

						ImGui::Separator();

						if (ImGui::MenuItem("Duplicate")) {
							Entity clonedEntity = entityMgr.CloneEntity(entity);
							SceneManager::GetInstance().AddEntityToScene(currentScene, clonedEntity);
						}
						if (ImGui::MenuItem("Delete")) {
							entityMgr.DestroyEntity(entity);
							if (static_cast<unsigned int>((selectedEntityIndex - 1)) == entity) {
								selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
								isEntitySelected_ = false;
							}
						}
						ImGui::EndPopup();
					}
				}
			}
			// End clipper
			clipper.End();

			// Child Window Context Menu
			if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {

				// Paste
				if (entityClipboard) {
					if (ImGui::MenuItem("Paste")) {
						PasteEntityFromClipboard();
					}
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("Paste");
					ImGui::PopStyleColor(1);
				}

				ImGui::Separator();

				// Prefabs
				if (ImGui::BeginMenu("Prefabs")) {
					// Platforms
					if (ImGui::BeginMenu("Platforms")) {
						for (const std::pair<std::string, PlatformPrefab>& pair : prefabSystem_.GetPlatformPrefabs()) {
							if (ImGui::MenuItem(pair.first.c_str())) {
								selectedPrefabType_ = PrefabType::Platform;
								selectedPrefabName_ = pair.first;
							}
						}
						ImGui::EndMenu();
					}
					// Trap Platforms
					if (ImGui::BeginMenu("Trap Platforms")) {
						for (const std::pair<std::string, TrapPlatformPrefab>& pair : prefabSystem_.GetTrapPlatformPrefabs()) {
							if (ImGui::MenuItem(pair.first.c_str())) {
								selectedPrefabType_ = PrefabType::TrapPlatform;
								selectedPrefabName_ = pair.first;
							}
						}
						ImGui::EndMenu();
					}
					// Scaling Platforms
					if (ImGui::BeginMenu("Scaling Platforms")) {
						for (const std::pair<std::string, ScalingPlatformPrefab>& pair : prefabSystem_.GetScalingPlatformPrefabs()) {
							if (ImGui::MenuItem(pair.first.c_str())) {
								selectedPrefabType_ = PrefabType::ScalingPlatform;
								selectedPrefabName_ = pair.first;
							}
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu(); // End Prefabs Menu
				}

				ImGui::Separator();

				// Create Empty Entity
				if (ImGui::MenuItem("Create Empty")) {
					Entity newEntity = entityMgr.CreateEntity();
					SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
				}
				ImGui::EndPopup();
			}
		}

		ImGui::EndChild();
		ImGui::End(); // Hierarchy End
	}

#pragma endregion

#pragma region INSPECTOR PANEL

	void Editor::ShowInspector() {
		using namespace Framework;

		//ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 400, 20), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowSize(ImVec2(380, ImGui::GetIO().DisplaySize.y - 100), ImGuiCond_FirstUseEver);

		ImGui::Begin("Inspector");
		if (isEntitySelected_) {
			EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
			Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);

			std::string& entityName = entityMgr.GetProperties(selectedEntity).name;

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
					size_t compIndex = static_cast<size_t>(selectedComponentIndex - 1); // Offset for "None"

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

		ImGui::End(); // End Inspector window
	}

	void Editor::ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent) {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		ImGui::SeparatorText(ComponentToString(selectedComponent).c_str());
		switch (selectedComponent) {
		case ComponentType::PositionType: {
			Position& position = entityMgr.GetComponent<Position>(selectedEntity);

			ImGui::InputFloat2("Position", &position.Pos.x, "%.2f");
			break;
		}
		case ComponentType::SpawnPointType: {
			SpawnPoint& spawnPt = entityMgr.GetComponent<SpawnPoint>(selectedEntity);

			ImGui::InputFloat2("Spawn Point", &spawnPt.Pos.x, "%.2f");
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
			ImGui::InputFloat2("HalfSize (X),(Y)", &col.HalfSize.x, "%.3f");
			break;
		}
		case ComponentType::ButtonType: {
			Button& buttonComp = entityMgr.GetComponent<Button>(selectedEntity);
			ImGui::Checkbox("Is Pressed?", &buttonComp.isPressed);
			const char* items[] = { "DEFAULT", "PLAYSOUND", "RESETPLAYERPOS" };
			int currentItemIndex = static_cast<int>(buttonComp.action);

			if (ImGui::Combo("Button Action", &currentItemIndex, items, IM_ARRAYSIZE(items))) {
				buttonComp.action = static_cast<ButtonAction>(currentItemIndex);
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
			TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);

			//Show text but make it read-only
			ImGui::InputText("Texture Key", &tex.textureKey, ImGuiInputTextFlags_ReadOnly);
			static std::string newTextureKey = "";
			ImGui::InputText("New Texture Key", &newTextureKey);

			//Button to update texture key
			if (ImGui::Button("Update Texture")) {
				if (!newTextureKey.empty()) {
					tex.textureKey = newTextureKey;
				}
			}

			// Vector2D HalfSize Input
			ImGui::InputFloat2("Half Size", &tex.HalfSize.x, "%.2f");

			break;
		}
		case ComponentType::AudioComponentType: {
			AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(selectedEntity);
			//Show text but make it read-only
			ImGui::InputText("Audio Key", &audioComp.soundKey, ImGuiInputTextFlags_ReadOnly);
			static std::string newSoundKey = "";
			ImGui::InputText("New Audio Key", &newSoundKey);
			//Button to update audio key
			if (ImGui::Button("Update Audio")) {
				if (!newSoundKey.empty()) {
					audioComp.soundKey = newSoundKey;
				}
			}
			ImGui::Checkbox("Looping", &audioComp.loop);
			ImGui::Checkbox("Stream", &audioComp.stream);
			ImGui::SliderFloat("Volume", &audioComp.volume, 0.0f, 1.0f, "%.2f");
			break;
		}

		case ComponentType::AnimationComponentType: {
			ImGui::TextDisabled("Properties are Hidden");
			break;
		}
		case ComponentType::TextComponentType: {
			TextComponent& textComp = entityMgr.GetComponent<TextComponent>(selectedEntity);

			// Text you want to draw
			char textBuffer[256] = "";

			// Copy textToDraw to textBuffer if !empty() && textBuffer != textToDraw
			if (!textComp.textToDraw.empty() && std::strcmp(textBuffer, textComp.textToDraw.c_str()) != 0) {
				std::strncpy(
					textBuffer,
					textComp.textToDraw.c_str(),
					IM_ARRAYSIZE(textBuffer) - 1
				);
				textBuffer[IM_ARRAYSIZE(textBuffer) - 1] = '\0';
			}

			if (ImGui::InputText("Text", textBuffer, IM_ARRAYSIZE(textBuffer))) {
				textComp.textToDraw = textBuffer;
			}

			// Font Selection
			char fontKeyBuffer[256] = "";

			// Copy fontKey to fontKeyBuffer if !empty() && fontKeyBuffer != textToDraw
			if (!textComp.fontKey.empty() && std::strcmp(fontKeyBuffer, textComp.fontKey.c_str()) != 0) {
				std::strncpy(
					fontKeyBuffer,
					textComp.fontKey.c_str(),
					IM_ARRAYSIZE(fontKeyBuffer) - 1
				);
				fontKeyBuffer[IM_ARRAYSIZE(fontKeyBuffer) - 1] = '\0';
			}

			if (ImGui::InputText("Font", fontKeyBuffer, IM_ARRAYSIZE(fontKeyBuffer))) {
				textComp.fontKey = fontKeyBuffer;
			}

			// Color Input
			ImGui::Text("Color RGBA: ");
			ImGui::InputFloat("R", &textComp.color.r, 0.f, 0.f, "%.3f");
			ImGui::InputFloat("G", &textComp.color.g, 0.f, 0.f, "%.3f");
			ImGui::InputFloat("B", &textComp.color.b, 0.f, 0.f, "%.3f");
			ImGui::InputFloat("A", &textComp.color.a, 0.f, 0.f, "%.3f");

			// Font Size Input
			ImGui::InputFloat("Font Size", &textComp.scale, 0.f, 0.f, "%.3f");

			break;
		}

		case ComponentType::BirdAIComponentType: {
			BirdAIComponent& ai = entityMgr.GetComponent<BirdAIComponent>(selectedEntity);

			// --- State (Read-only) ---
			int state = static_cast<int>(ai.state);
			const char* stateNames[] = { "IDLE", "WAKING", "CHASE", "HOVER", "ATTACK" };
			ImGui::Text("Current State: %s", stateNames[state]);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("State Timer", &ai.stateTimer, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Attack Timer", &ai.attackTimer, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputInt("Current Hover Node", &ai.currentHoverNode, 0, 0, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);

			ImGui::Separator();

			// --- Config ---
			ImGui::Text("Configurable Stats");
			ImGui::InputFloat("Wake-Up Radius", &ai.wakeUpRadius, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Hover Radius (Arrival)", &ai.hoverRadius, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Attack Radius", &ai.attackRadius, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Move Speed", &ai.moveSpeed, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Attack Speed", &ai.attackSpeed, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Attack Duration", &ai.attackDuration, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Attack Cooldown", &ai.attackCooldown, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Wake-Up Time", &ai.wakeUpTime, 0.05f, 0.1f, "%.2f");

			// --- Hover Config ---
			ImGui::Separator();
			ImGui::Text("Hover Behavior");
			ImGui::InputFloat("Hover Height", &ai.hoverHeight, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Hover Patrol Radius", &ai.hoverRadius_Circle, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Hover Node Reach Dist", &ai.hoverNodeReachedDist, 0.01f, 0.05f, "%.2f");
			ImGui::InputFloat("Hover Speed", &ai.hoverSpeed, 0.05f, 0.1f, "%.2f");

			// --- Pathfinding ---
			ImGui::Separator();
			ImGui::Text("Pathfinding");
			ImGui::InputFloat("Path Recalculate Interval", &ai.pathRecalculateInterval, 0.05f, 0.1f, "%.2f");

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Path Recalculate Timer", &ai.pathRecalculateTimer, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat2("Attack Target Pos", &ai.attackTargetPosition.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			ImGui::Text("Current Path Waypoints: %zu", ai.currentPath.size());

			break;
		}

		case ComponentType::SkeletonAIComponentType: {
			SkeletonAIComponent& skelAI = entityMgr.GetComponent<SkeletonAIComponent>(selectedEntity);

			// for now just show state
			int state = static_cast<int>(skelAI.state);
			ImGui::InputInt("State (0=Idle, 1=Chase)", &state, 0, 0, ImGuiInputTextFlags_ReadOnly);

			ImGui::InputFloat("Detection Radius", &skelAI.detectionRadius, 0.05f, 0.1f, "%.2f");
			ImGui::InputFloat("Move Speed", &skelAI.moveSpeed, 0.01f, 0.05f, "%.2f");
			break;
		}

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

#pragma endregion

#pragma region GAME VIEW PANEL

	void Editor::UpdateViewportData() {
		// This function just "peeks" at the "Game" window's state
		// This ensures our viewport data is up-to-date *before* any logic.

		ImGuiWindow* gameWindow = ImGui::FindWindowByName("Game");
		if (!gameWindow || gameWindow->Collapsed) {
			isMouseOverGameViewport_ = false;

			if (gameManager_->GetSystems().guiSystem_) { // Check if gui system exists
				gameManager_->GetSystems().guiSystem_->SetViewportContext({ 0,0 }, { 1,1 });
			}

			return; // Window not visible or found
		}

		// Get the absolute screen position of the content region's top-left corner
		gameViewportPos_ = gameWindow->ContentRegionRect.Min;

		// Get the size of the content region
		gameViewportSize_ = gameWindow->ContentRegionRect.GetSize();

		// Check if mouse is over the game viewport
		ImVec2 mousePos = ImGui::GetMousePos(); // ImGui::GetMousePos() gives absolute screen coords
		isMouseOverGameViewport_ =
			mousePos.x >= gameViewportPos_.x &&
			mousePos.x <= gameViewportPos_.x + gameViewportSize_.x &&
			mousePos.y >= gameViewportPos_.y &&
			mousePos.y <= gameViewportPos_.y + gameViewportSize_.y;

		if (gameManager_->GetSystems().guiSystem_) {
			gameManager_->GetSystems().guiSystem_->SetViewportContext(gameViewportPos_, gameViewportSize_);
		}
	}
	void Editor::ShowGameViewport() {
		using namespace Framework;

		ImGui::Begin("Game");

		ImVec2 viewportPanelSize = gameViewportSize_;

		if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
			Graphics* gfx = gameManager_->GetSystems().gfx_;
			SpriteRenderSystem* spriteRenderer_ = gameManager_->GetSystems().spriteRenderer_;
			CameraSystem* camSystem = gameManager_->GetSystems().camera_;

			if (gfx && spriteRenderer_ && camSystem) {

				// === STEP 0: UPDATE CAMERA ASPECT RATIO ===
				camSystem->UpdateAspectRatio(viewportPanelSize.x, viewportPanelSize.y);

				// === STEP 1: PREPARE FRAMEBUFFER ===
				gfx->ResizeGameFramebuffer(static_cast<unsigned int>(viewportPanelSize.x), static_cast<unsigned int>(viewportPanelSize.y));
				gfx->BindGameFramebuffer();
				gfx->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f }); // clear frame buffer first

				// === STEP 2: TEMPORARILY SWITCH TO EDITOR CAMERA ===
				Entity prevCamera = camSystem->GetActiveCamera();

				// Save current editor camera state before any rendering
				if (editorCamera_ != 0 && engine_->IsEditorMode()) {
					SaveEditorCameraState();
				}

				if (engine_->IsEditorMode() && editorCamera_ != 0) {
					camSystem->SetActiveCamera(editorCamera_);
				}

				// === STEP 3: RENDER SPRITES ===
				spriteRenderer_->Update();

				// === STEP 4: RESTORE ORIGINAL CAMERA ===
				if (engine_->IsEditorMode()) {
					camSystem->SetActiveCamera(prevCamera);

					// Ensure editor camera state is preserved
					if (prevCamera == editorCamera_ && editorCamera_ != 0) {
						RestoreEditorCameraState();
					}
				}

				// === STEP 5: UNBIND FRAMEBUFFER ===
				gfx->UnbindGameFramebuffer();

				// === STEP 6: DRAW GAME TEXTURE ===
				ImGui::Image((void*)(intptr_t)gfx->GetGameTexture(),
					viewportPanelSize,
					ImVec2(0, 1), ImVec2(1, 0));

				// === STEP 7: DRAW OVERLAYS (GRID, GIZMOS, DEBUG) ===
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImVec2 clipMin = gameViewportPos_;
				ImVec2 clipMax = ImVec2(gameViewportPos_.x + gameViewportSize_.x,
					gameViewportPos_.y + gameViewportSize_.y);
				drawList->PushClipRect(clipMin, clipMax, true);

			/*	if (showGrid_) {
					DrawEditorGrid(drawList);
				}*/

				if (isDraggingPrefab_ && isMouseOverGameViewport_) {
					DrawDragDropPreview(drawList);
				}

				AISystem* aiSystem = gameManager_->GetSystems().aiSystem_;
				if (aiSystem && camSystem && aiSystem->debugVisualization) {
					Entity activeCam = engine_->IsEditorMode() && editorCamera_ != 0
						? editorCamera_
						: camSystem->GetActiveCamera();

					aiSystem->DrawDebug(camSystem, drawList, this, activeCam);
				}

				drawList->PopClipRect();
			}
		}

		ImGui::End();
	}



#pragma endregion

#pragma region TOOLBAR

	void Editor::ShowToolbar() {
		// --- Style setup ---
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.0f));

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		// --- Begin docked window ---
		if (ImGui::Begin("Toolbar", nullptr, windowFlags))
		{
			PhysicsSystem* physics_ = gameManager_->GetSystems().physics_;

			// Align Center the Toolbar buttons
			float regionWidth = ImGui::GetContentRegionAvail().x; // Gets available width inside the docked window
			const float buttonWidth = 40.0f;
			const float buttonHeight = 20.0f;
			const float spacing = 10.0f;
			const float totalWidth = (buttonWidth * 3) + (spacing * 2);

			float offsetX = (regionWidth - totalWidth) * 0.5f;
			if (offsetX > 0.0f) {
				ImGui::SetCursorPosX(offsetX);
			}

			// Play Button
			ImGui::PushStyleColor(ImGuiCol_Button, isGamePlaying ? ImVec4(0.1f, 0.6f, 1.0f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

			EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

			// Start with physics paused
			if (!isGamePlaying) {
				physics_->SetPaused(true);
			}

			// Lock game keyboard if !isGamePlaying
			gameManager_->GetSystems().input_->SetKeyboardLocked(!isGamePlaying);

			
			if (ImGui::Button(isGamePlaying ? "Stop" : "Play", ImVec2(buttonWidth, buttonHeight)))
			{
				if (isGamePlaying) {
					gameManager_->GetSystems().levelResetter_->RunReset();
				}

				isGamePlaying = !isGamePlaying;
				isGamePaused = false;

				if (physics_->IsInStepMode()) physics_->ToggleStepMode();
				if (physics_->IsPaused()) physics_->SetPaused(false);
			}
			ImGui::PopStyleColor();

			ImGui::SameLine(0, spacing);

			// Pause Button
			ImGui::PushStyleColor(ImGuiCol_Button, isGamePaused ? ImVec4(0.1f, 0.6f, 1.0f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			if (!isGamePlaying) ImGui::BeginDisabled();
			if (ImGui::Button("Pause", ImVec2(buttonWidth, buttonHeight)))
			{
				if (physics_->IsInStepMode()) {
					physics_->ToggleStepMode();
				}
				else {
					physics_->SetPaused(!physics_->IsPaused());
				}

				isGamePaused = physics_->IsPaused();
			}
			if (!isGamePlaying) ImGui::EndDisabled();
			ImGui::PopStyleColor();

			ImGui::SameLine(0, spacing);

			// Stop Button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			if (!isGamePlaying) ImGui::BeginDisabled();
			if (ImGui::Button("Step", ImVec2(buttonWidth, buttonHeight)))
			{
				if (!physics_->IsInStepMode()) {
					physics_->ToggleStepMode();
					physics_->ExecuteNextStep();
					isGamePaused = true;
				}
				else {
					physics_->ExecuteNextStep();
				}
			}
			if (!isGamePlaying) ImGui::EndDisabled();
			ImGui::PopStyleColor();

			ImGui::End();
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

#pragma endregion

#pragma region EDITOR GRID
	void Editor::DrawEditorGrid(ImDrawList* drawList)
	{
		using namespace Framework;
		if (!cameraSystem_ || editorCamera_ == 0)
			return;

		const Camera& cam = cameraSystem_->GetCamera(editorCamera_);
		const Vector2D camPos = cam.position;

		// Match grid to viewport aspect
		float aspect = gameViewportSize_.x / gameViewportSize_.y;
		const float cellX = (baseGridCellSize_) / aspect;
		const float cellY = baseGridCellSize_;

		float halfW = cam.size.x * 0.5f;
		float halfH = cam.size.y * 0.5f;
		float left = camPos.x - halfW;
		float right = camPos.x + halfW;
		float bottom = camPos.y - halfH;
		float top = camPos.y + halfH;

		const float startX = std::floor(left / cellX) * cellX;
		const float startY = std::floor(bottom / cellY) * cellY;

		const ImU32 lineColor = IM_COL32(150, 150, 150, 90);
		const ImU32 axisXColor = IM_COL32(255, 0, 0, 160);
		const ImU32 axisYColor = IM_COL32(0, 255, 0, 160);

		for (float x = startX; x <= right; x += cellX)
		{
			Vector2D worldA = { x, bottom };
			Vector2D worldB = { x, top };
			ImVec2 a = WorldToImGuiScreen(worldA);
			ImVec2 b = WorldToImGuiScreen(worldB);
			drawList->AddLine(a, b, lineColor);
		}

		for (float y = startY; y <= top; y += cellY)
		{
			Vector2D worldA = { left, y };
			Vector2D worldB = { right, y };
			ImVec2 a = WorldToImGuiScreen(worldA);
			ImVec2 b = WorldToImGuiScreen(worldB);
			drawList->AddLine(a, b, lineColor);
		}

		Vector2D xA = { 0.0f, bottom };
		Vector2D xB = { 0.0f, top };
		Vector2D yA = { left, 0.0f };
		Vector2D yB = { right, 0.0f };

		drawList->AddLine(WorldToImGuiScreen(xA), WorldToImGuiScreen(xB), axisXColor, 2.0f);
		drawList->AddLine(WorldToImGuiScreen(yA), WorldToImGuiScreen(yB), axisYColor, 2.0f);
	}


	Vector2D Editor::SnapToGrid(const Vector2D& worldPos)
	{
		const float cell = baseGridCellSize_; // world units
		float snappedX = std::round(worldPos.x / cell) * cell;
		float snappedY = std::round(worldPos.y / cell) * cell;
		return { snappedX, snappedY };
	}


#pragma endregion

#pragma region ASSET BROWSER PANEL

	void Editor::ShowAssetBrowser(std::vector<std::filesystem::path>& droppedFiles) {
		assetBrowser.RenderUI(droppedFiles);
	}

#pragma endregion

#pragma endregion


	ImVec2 Editor::WorldToImGuiScreen(const Vector2D& worldPos, Entity cameraID)
	{
		if (!cameraSystem_ || cameraID == 0)
			return ImVec2(0, 0);
		
		// Validate that the entity is a camera
		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);

		if (!entityMgr.HasComponent(cameraID, Framework::CameraType)) {
			return ImVec2(0, 0);
		}

		const Camera& cam = cameraSystem_->GetCamera(cameraID); // USE THE PASSED CAMERA

		// Compute offset from camera center
		Vector2D offset = {
			(worldPos.x - cam.position.x) / (cam.size.x * 0.5f),
			(worldPos.y - cam.position.y) / (cam.size.y * 0.5f)
		};

		// Convert from world normalized (-1..1) to ImGui pixel coordinates
		float screenX = gameViewportPos_.x + ((offset.x + 1.0f) * 0.5f * gameViewportSize_.x);
		float screenY = gameViewportPos_.y + ((1.0f - (offset.y + 1.0f) * 0.5f) * gameViewportSize_.y);

		return ImVec2(screenX, screenY);
	}


	ImVec2 Editor::WorldToImGuiScreen(const Vector2D& worldPos)
	{
		// use overloaded function, defaulting to the editor camera
		return WorldToImGuiScreen(worldPos, editorCamera_);
	}


	void Editor::ShowAIDebugPanel() {
		if (!ImGui::Begin("AI Debug Visualization")) {
			ImGui::End();
			return;
		}

		// Get the AI system from game manager
		// !! IMPORTANT: This assumes you have added aiSystem_ to your GameManager systems struct
		AISystem* aiSystem = gameManager_->GetSystems().aiSystem_;

		if (!aiSystem) {
			ImGui::TextDisabled("AI System not available");
			ImGui::End();
			return;
		}

		// Master toggle
		ImGui::Checkbox("Enable AI Debug Visualization", &aiSystem->debugVisualization);
		ImGui::Separator();

		// Individual toggles
		ImGui::BeginDisabled(!aiSystem->debugVisualization);

		ImGui::Text("Visualization Options:");
		ImGui::Checkbox("Show Navigation Grid", &aiSystem->showNavigationGrid);
		ImGui::Checkbox("Show Bird Debug Info", &aiSystem->showBirdDebug);
		ImGui::Checkbox("Show Paths", &aiSystem->showPaths);
		ImGui::Checkbox("Show Colliders", &aiSystem->showColliders);

		ImGui::EndDisabled();

		ImGui::Separator();

		// Color legend
		if (ImGui::CollapsingHeader("State Colors Legend")) {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Gray = IDLE");
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow = WAKING");
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Orange = CHASE");
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Cyan = HOVER");
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red = ATTACK");
		}

		if (ImGui::CollapsingHeader("Radius Legend")) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "Yellow Circle = Wake-up Radius");
			ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.7f, 1.0f), "Cyan Circle = Hover Radius");
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red Circle = Attack Radius");
		}

		if (ImGui::CollapsingHeader("Grid Legend")) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Green Cells = Walkable");
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red Cells = Blocked");
		}

		if (ImGui::CollapsingHeader("Path Legend")) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Magenta Line = Current Path");
			ImGui::Text("Squares = Waypoints");
		}

		ImGui::End();
	}
}