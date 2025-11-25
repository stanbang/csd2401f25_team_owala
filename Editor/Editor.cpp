/*********************************************************************//**
* \file   Editor.cpp
*
* \primary_author
*		Jason Lou (19%)
* \secondary_author
*		Lur Pei Hao Stanley (19%)
*		Sitara Anil (19%)
*		Ng Jordain Ying Zhe (19%)
*		Yar Xuan (19%)
*		Ng Jing Wen (5%)
*		
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
#include "rapidjson/prettywriter.h"

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

// Native Scripting
#include "ScriptableEntity.h"
#include "ScriptRegistry.h"
#include "NativeScriptMaker.h"

#include <unordered_set>
// Standard Libraries
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Internal Helpers
namespace Editor {
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
		case ComponentType::TextureComponentType:			return "Texture";
		case ComponentType::AudioComponentType:				return "Audio";
		case ComponentType::AnimationComponentType:			return "Animation";
		case ComponentType::TextComponentType:				return "Text";
		case ComponentType::CameraType:						return "Camera";
		case ComponentType::NativeScriptsComponentType:		return "Scripts";
		case ComponentType::ButtonType:						return "Button";
		case ComponentType::BirdAIComponentType:			return "BirdAI";
		case ComponentType::SkeletonAIComponentType:		return "SkeletonAI";
		case ComponentType::ParticleEmitterComponentType:	return "ParticleEmitter";
		case ComponentType::ParticleComponentType:			return "Particle";
		case ComponentType::PlayerTagType:					return "PlayerTag";
		case ComponentType::PlatformTagType:				return "PlatformTag";
		case ComponentType::BackgroundTagType:				return "BackgroundTag";
		case ComponentType::EnemyTagType:					return "EnemyTag";
		case ComponentType::SceneOwnedType:					return "SceneOwned";
		case ComponentType::PersistentTagType:				return "PersistentTag";
		case ComponentType::ActiveTagType:					return "ActiveTag";
		case ComponentType::GUITagType:						return "GUITag";
		case ComponentType::RotationType:                   return "Rotation";
		case ComponentType::ScaleType:                      return "Scale";
		
		default:											return "Unknown";
		}
	}

	// Helper to convert a CollisionLayer enum to its string representation.
	char const* LayerToString(CollisionLayer layer) {
		switch (layer) {
		case CollisionLayer::Default:   return "Default";
		case CollisionLayer::Player:    return "Player";
		case CollisionLayer::Platform:  return "Platform";
		case CollisionLayer::Enemy:     return "Enemy";
		case CollisionLayer::Trigger:   return "Trigger";
		default:                        return "Layer  Unknown";
		}
	}
}

namespace Editor {
	void EditSession::Begin(Entity entity, Framework::ComponentType compType) {
		using namespace Framework;
		EntityStorage* ecs = GameManager::GetInstance().GetSystems().ecs_;

		active = true;
		entityInEdit = entity;
		compInEdit = compType;

		beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
	}

	void EditSession::End(Entity entity, Framework::ComponentType compType, const std::string& description, HistoryManager& historyManager_) {
		if (active && entityInEdit == entity && compInEdit == compType) {
			EntityStorage* ecs = GameManager::GetInstance().GetSystems().ecs_;
			EntityManager entityMgr(*ecs);

			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

			if (beforeJSON != afterJSON) {
				historyManager_.RecordModifyComponent(entity, description, beforeJSON, afterJSON, compType);
			}

			*this = {};
		}
	}

	// Add these member variable definitions at the top of your Editor class implementation
	// (or wherever you have other member variable initializations)

	// In the constructor, initialize the camera state:
	Editor::Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm)
		: engine_(&engine), gameManager_(&gameManager), resourceManager_(&rcm), assetBrowser(rcm),
		cameraSystem_(gameManager_->GetSystems().camera_),
		graphicsSystem_(gameManager_->GetSystems().gfx_),
		transformationSystem_(gameManager_->GetSystems().transformation_),
		defaultEditorCameraState_{ {0.0f, 0.0f}, 1.0f },
		currentEditorCameraState_{ {0.0f, 0.0f}, 1.0f },
		editorCameraNeedsReset_(false),
		historyManager_(*gameManager_->GetSystems().ecs_) {
	};

	// Add these implementations to your Editor.cpp file

	void Editor::ResetEditorCamera() {
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			cam.position = defaultEditorCameraState_.position;
			cam.zoom = defaultEditorCameraState_.zoom;
			cameraSystem_->SetZoom(editorCamera_, 540.0f); //540 for 1080p

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

			/*Framework::Logger::Get().Info(DebugType::Editor)
				<< "Saved editor camera state: position ("
				<< currentEditorCameraState_.position.x << ", " << currentEditorCameraState_.position.y
				<< ") zoom: " << currentEditorCameraState_.zoom << std::endl;*/
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
		if (!prefabSystem_.LoadPrefabsFromFolder("Data/Prefabs/")) {
			Framework::Logger::Get().Error(DebugType::Editor)
				<< "Failed to load prefabs from Data/prefabs/" << std::endl;
		}
		else {
			Framework::Logger::Get().Info(DebugType::Editor)
				<< "Successfully loaded prefabs from folder" << std::endl;

			if (Framework::SceneManager::GetInstance().GetActiveScene().IsValid()) {
				ApplyAllPrefabChangesToCurrentScene();
			}
		}

		LoadTilesFromFile();
		// --- Prepare Palette Items (Load Textures) ---
		// (assuming this stays as-is)
		firstUpdate = true;


		cameraSystem_ = gameManager_->GetSystems().camera_;
		graphicsSystem_ = gameManager_->GetSystems().gfx_;
		transformationSystem_ = gameManager_->GetSystems().transformation_;
		if (!transformationSystem_) {
			Framework::Logger::Get().Error(DebugType::Editor)<< "Transformation system not available" << std::endl;
		}

		// In your Initialize() function, update camera setup:
		if (cameraSystem_ && editorCamera_ == 0) {
			editorCamera_ = cameraSystem_->CreateCamera({ 0.0f, 0.0f }, 0, false);

			// Set appropriate world space size for the camera
			Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			cam.size = { 20.0f, 20.0f }; // Adjust based on your world scale
			cam.zoom = 1.0f;

			cameraSystem_->SetZoom(editorCamera_, 540.0f); //540 for 1080p
			float aspect = gameViewportSize_.x / gameViewportSize_.y;
			cameraSystem_->SetAspectRatio(editorCamera_, aspect);

			// Update default state
			defaultEditorCameraState_.position = { 0.0f, 0.0f };
			defaultEditorCameraState_.zoom = 1.0f;
			currentEditorCameraState_ = defaultEditorCameraState_;
		}

		initialized = true;
		engine_->SetMode(EngineMode::Editor);
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

		if (!tilePalettePrepared_ && resourceManager_) {
			Framework::Logger::Get().Info(DebugType::Editor)<< "Running PrepareTilePaletteItems..." << std::endl;
			PrepareTilePaletteItems();
			tilePalettePrepared_ = true;
			Framework::Logger::Get().Info(DebugType::Editor)<< "PrepareTilePaletteItems finished. tilePaletteItems_ size: "<< tilePaletteItems_.size() << std::endl;
		}

		//for IMGUI INPUT
		//ImGuiIO& io = ImGui::GetIO();

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
			ImGui::DockBuilderDockWindow("Layer", dock_right);

			ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.35f, nullptr, &dock_right);
			ImGui::DockBuilderDockWindow("Performance Profiler", dock_right_bottom);

			ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.35f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Asset Viewer", dock_bottom);
			ImGui::DockBuilderDockWindow("Prefab Palette", dock_bottom);
			ImGui::DockBuilderDockWindow("Prefab Editor", dock_bottom);
			ImGui::DockBuilderDockWindow("Tile Palette", dock_bottom);
			ImGuiID dock_bottom_left = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.2f, nullptr, &dock_bottom);
			ImGui::DockBuilderDockWindow("Project Explorer", dock_bottom_left);

			ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
			ImGui::DockBuilderDockWindow("Debug Visualization", dock_left);
			ImGui::DockBuilderDockWindow("Level Editor", dock_left);
			ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
			

			ImGui::DockBuilderDockWindow("Game", dock_main_id);
			ImGui::DockBuilderFinish(dockspaceID);
		}

		// --- UPDATE VIEWPORT DATA ---
		UpdateViewportData();

		gameManager_->GetSystems().input_->SetCursorLocked(false);

		// --- Handle Editor Logic ---
		HandleSpawning();
		HandleBrushInput();
		HandleSelection();
		HandleDeletion();
		
		if (isTileEditorActive_ && isDraggingPrefab_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			HandleDragDropPlacement();
		}
		if (isTileEditorActive_ && dragDropPayload_.type == PrefabType::Eraser) {
			HandleTileErasing();
		}
		if (!isDraggingPrefab_ && dragDropPayload_.type != PrefabType::Eraser && !isBrushMode_) {
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
		if (settings.showLayerManager) ShowLayerManager();
		if (isTileEditorActive_) {
			ShowTilemapEditor();  // Prefab palette
			if (isTilePaletteOpen_) {
				ShowTilePalette(); // Tile palette
			}
		}

		if (settings.showGameViewport) ShowGameViewport();
		ShowDebugVisualizationPanel(); 
		if (isPrefabEditorOpen_) ShowPrefabEditor();

		// Set editor play/edit mode
		if (isTileEditorActive_ || !isGamePlaying || (gizmoSystem_ && gizmoSystem_->IsGizmosToggled())) {
			engine_->SetMode(EngineMode::Editor);
		}
		else if (isGamePlaying) {
			engine_->SetMode(EngineMode::Play);
		}

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
					paletteItems_.push_back({ name, type, texID , ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f) });
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

		Framework::Logger::Get().Info(DebugType::Editor) << "Prepared " << paletteItems_.size() << " palette items." << std::endl;
	}

	Vector2D Editor::GetMouseWorldPosition()
	{
		if (!cameraSystem_ || editorCamera_ == 0)
			return { 0.0f, 0.0f };

		// Get mouse pos in ImGui screen space
		ImVec2 mouse = ImGui::GetMousePos();

		// Convert to viewport-relative coords
		float relX = mouse.x - gameViewportPos_.x;
		float relY = mouse.y - gameViewportPos_.y;

		if (relX < 0 || relY < 0 ||
			relX > gameViewportSize_.x || relY > gameViewportSize_.y)
			return { 0.0f, 0.0f };

		// Get camera properties
		const Camera& cam = cameraSystem_->GetCamera(editorCamera_);

		// Convert screen coordinates to world coordinates
		float worldX = cam.position.x + ((relX / gameViewportSize_.x) - 0.5f) * cam.size.x;
		float worldY = cam.position.y + ((1.0f - (relY / gameViewportSize_.y)) - 0.5f) * cam.size.y;

		return { worldX, worldY };
	}

	Vector2D Editor::GetNormalizedMousePos() const {
		ImVec2 absoluteMousePos = ImGui::GetIO().MousePos;
		double mouseX = static_cast<double>(absoluteMousePos.x - gameViewportPos_.x);
		double mouseY = static_cast<double>(absoluteMousePos.y - gameViewportPos_.y);
		double mouseX_ndc = static_cast<double>((mouseX / gameViewportSize_.x) * 2.0f - 1.0f);
		double mouseY_ndc = static_cast<double>(1.0f - (mouseY / gameViewportSize_.y) * 2.0f);
		return Vector2D(static_cast<float>(mouseX_ndc), static_cast<float>(mouseY_ndc));
	}


	void Editor::HandleDragDropPlacement()
	{
		if (!isDraggingPrefab_ || !isMouseOverGameViewport_) {
			isDraggingPrefab_ = false;
			return;
		}

		//const Camera& cam = cameraSystem_->GetCamera(editorCamera_);

		// --- MOUSE POSITION (ImGui → World) ---
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 viewportPos = gameViewportPos_;
		ImVec2 viewportSize = gameViewportSize_;


		float screenX = ((mousePos.x - viewportPos.x) / viewportSize.x) * 2.0f - 1.0f;
		float screenY = 1.0f - ((mousePos.y - viewportPos.y) / viewportSize.y) * 2.0f;

		Vector2D screenPos = { screenX, screenY };
		Vector2D worldMousePos = cameraSystem_->ScreenToWorld(screenPos, editorCamera_);


		// --- SNAP TO GRID ---
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		// Call the new helper
		PlaceObjectInWorld(dragDropPayload_, snappedPos);

		isDraggingPrefab_ = false;

	}


	void Editor::DrawDragDropPreview(ImDrawList* drawList)
	{

		// 1. Check Visibility
		bool showPreview = (isDraggingPrefab_ || isBrushMode_) && isMouseOverGameViewport_;
		if (!showPreview) return;

		// 2. Get Payload
		const DraggablePrefabInfo& currentPayload = isDraggingPrefab_ ? dragDropPayload_ : brushPayload_;
		if (currentPayload.type == PrefabType::None || currentPayload.type == PrefabType::Eraser) return;

		// DEBUG: Log what we're trying to draw
		static int debugCounter = 0;
		if (debugCounter++ % 60 == 0) { // Log once per second at 60fps
			Framework::Logger::Get().Info(DebugType::Editor)
				<< "PREVIEW: " << currentPayload.name
				<< " | TexID: " << currentPayload.textureID
				<< " | UV0: (" << currentPayload.uv0.x << "," << currentPayload.uv0.y << ")"
				<< " | UV1: (" << currentPayload.uv1.x << "," << currentPayload.uv1.y << ")" << std::endl;
		}


		

		if (!cameraSystem_ || editorCamera_ == 0) return;

		// 4. Calculate Position (Snapped to Grid)
		Vector2D worldMousePos = GetMouseWorldPosition();
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		// Update internal state vars for logic elsewhere
		if (isDraggingPrefab_) dragDropPreviewPos_ = snappedPos;
		else brushPreviewPos_ = snappedPos;

		// 5. Calculate Screen Rect
		Vector2D halfSize = { baseGridCellSize_, baseGridCellSize_ };

		Vector2D worldMin = { snappedPos.x - halfSize.x, snappedPos.y - halfSize.y };
		Vector2D worldMax = { snappedPos.x + halfSize.x, snappedPos.y + halfSize.y };

		ImVec2 p1 = WorldToImGuiScreen(worldMin);
		ImVec2 p2 = WorldToImGuiScreen(worldMax);

		// FIX: Normalize Min/Max to prevent negative width/height
		ImVec2 rectMin = ImVec2(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
		ImVec2 rectMax = ImVec2(std::max(p1.x, p2.x), std::max(p1.y, p2.y));

		// 6. Draw with proper clipping
		ImVec2 clipMin = gameViewportPos_;
		ImVec2 clipMax = ImVec2(gameViewportPos_.x + gameViewportSize_.x,
			gameViewportPos_.y + gameViewportSize_.y);

		// Only draw if within viewport bounds
		if (rectMin.x < clipMax.x && rectMax.x > clipMin.x &&
			rectMin.y < clipMax.y && rectMax.y > clipMin.y) {

			ImU32 tint = isBrushMode_ ? IM_COL32(255, 255, 255, 200) : IM_COL32(255, 255, 255, 128);

			// Cast texture ID properly for ImGui
			ImTextureID texID = (ImTextureID)(intptr_t)currentPayload.textureID;

			//ImTextureID texID = (ImTextureID)(intptr_t)previewTextureID;

			// Draw the image
			drawList->AddImage(
				texID,
				rectMin,
				rectMax,
				currentPayload.uv0,
				currentPayload.uv1,
				tint
			);

			// Draw border
		/*	ImU32 borderCol = isBrushMode_ ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255);
			drawList->AddRect(rectMin, rectMax, borderCol, 0.0f, 0, 2.0f);*/
		}
		//if (!isDraggingPrefab_ ||
		//	dragDropPayload_.type == PrefabType::None ||
		//	dragDropPayload_.type == PrefabType::Eraser)
		//	return;

		//if (!cameraSystem_ || editorCamera_ == 0)
		//	return;

		//Vector2D worldMousePos = GetMouseWorldPosition();
		//dragDropPreviewPos_ = SnapToGrid(worldMousePos);

		//Texture previewTextureID = dragDropPayload_.textureID;
		//if (previewTextureID == 0)
		//	return;

		//// Use consistent world space sizing
		//Vector2D previewHalfSize = { baseGridCellSize_, baseGridCellSize_ };

		//// Calculate world space corners
		//Vector2D worldMin = { dragDropPreviewPos_.x - previewHalfSize.x, dragDropPreviewPos_.y - previewHalfSize.y };
		//Vector2D worldMax = { dragDropPreviewPos_.x + previewHalfSize.x, dragDropPreviewPos_.y + previewHalfSize.y };

		//// Convert to screen space
		//ImVec2 screenMin = WorldToImGuiScreen(worldMin);
		//ImVec2 screenMax = WorldToImGuiScreen(worldMax);

		//ImVec2 uv0 = dragDropPayload_.uv0;
		//ImVec2 uv1 = dragDropPayload_.uv1;
		//ImU32 tintColor = IM_COL32(255, 255, 255, 150);

		//ImTextureID texID = (ImTextureID)(intptr_t)previewTextureID;
		//drawList->AddImage(texID, screenMin, screenMax, uv0, uv1, tintColor);
	}

	Vector2D Editor::GetPrefabHalfSize(const std::string& name, PrefabType type)
	{
		// Default size if nothing is found (e.g. 1 grid cell)
		Vector2D defaultSize = { baseGridCellSize_, baseGridCellSize_ };

		if (type == PrefabType::Tile) {
			// Tiles usually fit exactly in the grid
			return defaultSize;
		}

		// Look up the prefab definition
		const EntityPrefab* prefab = prefabSystem_.GetPrefab(name);
		if (!prefab) return defaultSize;

		// 1. Try to find AABBCollider data
		for (const auto& comp : prefab->components) {
			if (comp.type == "AABBCollider") {
				if (comp.data.HasMember("halfSizeX") && comp.data.HasMember("halfSizeY")) {
					return Vector2D{
						comp.data["halfSizeX"].GetFloat(),
						comp.data["halfSizeY"].GetFloat()
					};
				}
			}
		}

		// 2. Fallback to TextureComponent data if no collider
		for (const auto& comp : prefab->components) {
			if (comp.type == "TextureComponent") {
				if (comp.data.HasMember("halfSizeX") && comp.data.HasMember("halfSizeY")) {
					return Vector2D{
						comp.data["halfSizeX"].GetFloat(),
						comp.data["halfSizeY"].GetFloat()
					};
				}
			}
		}

		return defaultSize;
	}

	

	void Editor::PlaceObjectInWorld(const DraggablePrefabInfo& info, Vector2D position)
	{
		// 1. Force Snap (Double check)
		Vector2D snappedPos = SnapToGrid(position);

		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		// 2. Overlap Check
		// We check if ANY entity with a Position component exists at this exact grid point.
		// Epsilon is small to allow floating point comparison.
		const float epsilon = 0.05f;
		const std::vector<Entity>& entities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

		for (const auto& entity : entities) {
			if (entityMgr.HasComponent(entity, Framework::PositionType)) {
				const auto& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;

				// Check intersection
				if (std::abs(pos.x - snappedPos.x) < epsilon &&
					std::abs(pos.y - snappedPos.y) < epsilon) {

					// Allow replacing if in Eraser mode, otherwise BLOCK placement
					if (info.type != PrefabType::Eraser) {
						return; // EXIT: Object already exists here
					}
				}
			}
		}

		// 3. Create Entity
		Entity newEntity = 0;

		if (info.type == PrefabType::Tile) {
			newEntity = entityMgr.CreateEntity();

			// Setup Tile Components...
			Vector2D tileHalfSize = { baseGridCellSize_, baseGridCellSize_ };

			// Ensure texture key is valid (Try appending .png if raw name fails)
			std::string textureKey = info.name;
			if (!resourceManager_->has<Texture>(textureKey) && resourceManager_->has<Texture>(textureKey + ".png")) {
				textureKey += ".png";
			}

			entityMgr.AddComponent(newEntity, Framework::Position{ snappedPos });
			entityMgr.AddComponent(newEntity, Framework::AABBCollider{ tileHalfSize });
			entityMgr.AddComponent(newEntity, Framework::TextureComponent{ textureKey, tileHalfSize });
			entityMgr.AddComponent(newEntity, Framework::PlatformTag{});
			entityMgr.GetProperties(newEntity).name = info.name;
		}
		else if (info.type != PrefabType::Eraser) {
			// Create Prefab
			newEntity = prefabSystem_.CreateEntityFromPrefab(*ecs, info.name, snappedPos);
		}
		if (newEntity != 0) {
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);
			// Optional: Record History
			historyManager_.Record(OpType::Create, newEntity, "Brush Place");
		}
	}

	void Editor::HandleBrushInput()
	{
		// 1. Exit conditions
		if (!isBrushMode_ || isDraggingPrefab_) return;

		// 2. Cancellation (Right Click in viewport or ESC)
		Framework::InputSystem* input = gameManager_->GetSystems().input_;
		if (input->MousePressed(Framework::InputSystem::MouseButton::Right) ||
			input->KeyPressed(Framework::InputSystem::Key::Escape)) {
			isBrushMode_ = false;
			lastPlacedPos_ = { -99999.0f, -99999.0f };
			Framework::Logger::Get().Info(DebugType::Editor) << "Exited Brush Mode" << std::endl;
			return;
		}

		// 3. Painting Logic (Left Mouse Held)
		if (input->MouseDown(Framework::InputSystem::MouseButton::Left) && isMouseOverGameViewport_) {

			Vector2D worldMousePos = GetMouseWorldPosition();
			Vector2D snappedPos = SnapToGrid(worldMousePos);

			// check if we already placed at this exact grid spot this stroke
			// (Prevents spamming the creation function 60 times a second on one tile)
			if (snappedPos.x != lastPlacedPos_.x || snappedPos.y != lastPlacedPos_.y) {

				if (brushPayload_.type == PrefabType::Eraser) {
					// Set payload for existing eraser logic function, or call specific eraser helper
					dragDropPayload_ = brushPayload_; // Hacky reuse of existing eraser logic
					HandleTileErasing();
				}
				else {
					PlaceObjectInWorld(brushPayload_, snappedPos);
				}

				lastPlacedPos_ = snappedPos;
			}
		}
		else if (input->MouseReleased(Framework::InputSystem::MouseButton::Left)) {
			// Reset last placed when mouse is let go, allows clicking same spot again if needed
			lastPlacedPos_ = { -99999.0f, -99999.0f };
		}
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

		// If mouse over game viewport & not dragging prefab & not hovering on gizmos
		// check for mouse click on invisible entity placeholders / visible entities
		// and update the editor's selection state
		if (!isDraggingPrefab_ && gizmoSystem_ && !gizmoSystem_->IsGizmosHover()
			&& ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			// check if clicked on invisible entity
			Entity invisEntityClicked = gizmoSystem_->IsInvisEntityClicked();

			// an invisible entity was clicked
			if (invisEntityClicked > 0) {
				selectedEntityIndex = invisEntityClicked + 1;
				isEntitySelected_ = true;
				selectedComponentIndex = 0;
				selectedTagIndex = 0;
			}
			// else check entity clicked
			else {
				Entity bestCandidate = GetObjAtMousePos();
				if (bestCandidate > 0) {
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

	void Editor::CopyEntityToClipboard(const Entity entity) {
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
		historyManager_.Record(OpType::Create, pastedEntity, "Pasted Entity");
	}

	std::string Editor::SerializeEntityToString(const Entity entity) {
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

		return std::string(buffer.GetString());
	}

#pragma endregion

#pragma endregion

	// --- Panel Rendering ---
#pragma region INTERNAL PANELS

#pragma region PERFORMANCE PROFILER PANEL
	void Editor::ShowPerformanceProfile() {

		ImGui::Begin("Performance Profiler");
		{
			/*Framework::PhysicsSystem* physics = gameManager_->GetSystems().physics_;
			if (physics) {
				bool isForced = physics->IsForcingNarrowPhase();
				if (ImGui::Checkbox("Force Narrow-Phase (O(n^2))", &isForced)) {
					physics->SetNarrowPhase(isForced);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Disables broad-phase grid to show performance difference.");
				}
				ImGui::Separator();
			}*/

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
			SaveAndLoadSection();
		}
		else {
			ImGui::Text("Scene: %s", currentScene.name.c_str());
			ImGui::Text("Entities: %zu", entitiesLoaded.size());

			SaveAndLoadSection();

			// --- Entity Factory ---
			ImGui::SeparatorText("Entity Factory");

			if (ImGui::Button("Create Empty")) {
				Entity newEntity = entityMgr.CreateEntity();
				SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
				historyManager_.Record(OpType::Create, newEntity, "Create Entity");
			}

			ImGui::SameLine();
			if (ImGui::Button("Clone Selected Entity") && selectedEntityIndex > 0) {
				Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);
				Entity clonedEntity = entityMgr.CloneEntity(selectedEntity);
				SceneManager::GetInstance().AddEntityToScene(currentScene, clonedEntity);
				historyManager_.Record(OpType::Create, clonedEntity, "Clone Entity");
			}

			// --- Entity Inspector ---
			// Add reset camera button
			ImGui::SeparatorText("Reset Editor Camera");
			if (ImGui::Button("Reset Editor Camera")) {
				ResetEditorCamera();
			}

			ImGui::SeparatorText("Prefab/Tile Editors");
			// Button to activate the tile editor mode
			if (ImGui::Button("Open Tilemap Editor")) {
				isTileEditorActive_ = true;
				isTilePaletteOpen_ = true;
			}

			if (ImGui::Button("Open Prefab Editor")) {
				isPrefabEditorOpen_ = true;
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
		ImGui::SeparatorText("Spawn Game Objects");
		// In your Editor.cpp spawning section:
		if (ImGui::Button("Spawn 1000 Objects")) {
			Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
			Framework::ResourceManager* rcm = gameManager_->GetSystems().resourceManager_;

			if (ecs) {
				// Create a grid of 2500 entities spread out
				//int gridSize = 50; // 50x50 grid = 2500 entities
				//float spacing = 0.1f; // Adjust this for proper spacing

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
		//ImGui::SameLine();
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
			//entityMgr.AddComponent<RotationFlag>(bird, {});

			RigidBody rb;
			rb.SetMass(1.0f); // Give it some mass
			rb.isAffectedByGravity = false; // Make sure it doesn't fall
			entityMgr.AddComponent<RigidBody>(bird, rb);

			//  Add components to make it visible and selectable
			Vector2D texHalfSize = { 100.f, 100.f }; // Set a default size
			Vector2D aabbHalfSize = { 50.f, 50.f }; // Set a default size

			// texture is loaded in TestScene.cpp
			//entityMgr.AddComponent<TextureComponent>(bird, { "Eli_Idle_Spritesheet.png", halfSize });
			entityMgr.AddComponent<TextureComponent>(bird, { "Bird_Enemy_SleepIdle_Spritesheet.png", texHalfSize });
			
		    entityMgr.AddComponent<AABBCollider>(bird, { aabbHalfSize });

			// Add it to the active scene
			sceneMgr.AddEntityToScene(currScene, bird);

			Logger::Get().Info(DebugType::Editor) << "Spawned Test Bird (Entity ID: "
				<< bird << ")";
		}

		//ImGui::SameLine();
		if (ImGui::Button("Spawn Test Skeleton")) {
			
			SceneManager& sceneMgr = SceneManager::GetInstance();
			//const SceneAsset& currentScene = sceneMgr.GetActiveScene();

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
	
	void Editor::SaveScene() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		const SceneAsset& currentScene = sceneMgr.GetActiveScene();
		if (!currentScene.IsValid()) return;

		const std::vector<Entity>& entitiesLoaded = sceneMgr.GetEntitiesInScene(currentScene.name);

		// Reset entity states & stop game on save 
		isGamePlaying = false;
		gameManager_->GetSystems().levelResetter_->RunReset();
		std::string fileName = FileIOSystem::SanitizeFileName(currentScene.name) + ".json";
		std::string filePath = FileIOKeys::SCENES_FILEPATH_PREFIX + fileName;

		// If its a newly created scene, create new filepath for it based on scene name.
		if (currentScene.path.empty()) {
			sceneMgr.UpdateScenePath(currentScene.sceneIndex, filePath);
		}

		// Serialize Scene name and entities to its own json file in local Data folder
		EntitySerializer::SerializeSceneData(entitiesLoaded, *(gameManager_->GetSystems().ecs_), filePath, currentScene.name);

		// Check if root Data folder exists and serialize scene data to root folder
		if (std::filesystem::exists(FileIOKeys::ROOT_DATA_FOLDER)) {
			std::string rootFilePath = FileIOKeys::ROOT_FOLDER_PREFIX + filePath;
			EntitySerializer::SerializeSceneData(entitiesLoaded, *(gameManager_->GetSystems().ecs_), rootFilePath, currentScene.name);
		}

		// Save scene asset to json file that stores all scene assets
		FileIOSystem::SaveSceneAsset(currentScene);
	}

	void Editor::SaveAndLoadSection() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		const SceneAsset& currentScene = sceneMgr.GetActiveScene();
		bool isCurrentSceneValid = currentScene.IsValid();

		ImGui::SeparatorText("Scene Management");

		// --- Save (Overwrite/Quick Save. No changes to scene name or file path) ---
		if (!isCurrentSceneValid) ImGui::BeginDisabled();
		if (ImGui::Button("Save")) {
			SaveScene();
		}
		if (!isCurrentSceneValid) ImGui::EndDisabled();

		// --- Create New Scene --- 
		ImGui::SameLine();
		if (ImGui::Button("New Scene")) {
			ImGui::OpenPopup("Create New Scene");
		}

		// --- Unload Current Scene ---
		ImGui::SameLine();
		ImU32 buttonColor = IM_COL32(150, 50, 50, 255);
		ImU32 buttonHoverColor = IM_COL32(200, 50, 50, 255);
		ImU32 buttonActiveColor = IM_COL32(215, 50, 50, 255);

		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);

		if (!isCurrentSceneValid) ImGui::BeginDisabled();
		if (ImGui::Button("Unload Scene")) {
			ImGui::OpenPopup("Save");
		}
		ImGui::PopStyleColor(3);
		if (!isCurrentSceneValid) ImGui::EndDisabled();

		// Save Scene Modal (pop up before unloading current scene)
		if (ImGui::BeginPopupModal("Save", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Save the current scene before unloading?");

			if (ImGui::Button("Confirm", ImVec2(120, 0)))
			{
				SaveScene();
				ImGui::CloseCurrentPopup();
				sceneMgr.UnloadScene();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
				sceneMgr.UnloadScene();
			}

			ImGui::EndPopup();
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

				if (isCurrentScene || !scene.IsValid()) {
					ImGui::BeginDisabled();
				}

				if (ImGui::Button(scene.name.c_str())) {
					Logger::Get().Info(DebugType::Editor) << "Attempting to load scene: " << scene.name << std::endl;
					if (!isCurrentScene) {
						isGamePlaying = false;
						gameManager_->GetSystems().levelResetter_->RunReset();
						ResetEditorCamera();
						if (!prefabSystem_.LoadPrefabsFromFolder("Data/Prefabs/")) {
							Framework::Logger::Get().Error(DebugType::Editor)
								<< "Failed to reload prefabs before scene load!" << std::endl;
						}
						else {
							Framework::Logger::Get().Info(DebugType::Editor)
								<< "Successfully reloaded prefabs before scene load" << std::endl;
						}
						sceneMgr.LoadScene(id);
						ApplyAllPrefabChangesToCurrentScene();

						palettePrepared_ = false;
						tilePalettePrepared_ = false;
					}
				}

				if (isCurrentScene || !scene.IsValid()) {
					ImGui::EndDisabled();
				}
			}
		}
	}

#pragma endregion

#pragma region TILEMAP EDITOR PANEL
	void Editor::ShowTilePalette() {
		static bool wasTilePaletteOpen = false;
		if (ImGui::Begin("Tile Palette", &isTileEditorActive_)) {
			if (!wasTilePaletteOpen) {
				wasTilePaletteOpen = true;
			}

			ImGui::Text("Left Click to drag and drop, Right click to select item for Brush Mode");
			ImGui::Separator();

			showGrid_ = true;

			ImVec2 buttonSize(64, 64);
			ImVec4 bgCol(0, 0, 0, 0);
			ImVec4 tintCol(1, 1, 1, 1);

			// --- Eraser Tool ---
			if (ImGui::Selectable("Eraser", dragDropPayload_.type == PrefabType::Eraser && isDraggingPrefab_)) {
				isDraggingPrefab_ = true;
				dragDropPayload_ = { "Eraser", PrefabType::Eraser, 0 };
			}
			ImGui::Separator();

			// --- Tile Items ---
			int itemsPerRow = 0;
			const int maxItemsPerRow = 4;

			if (tilePaletteItems_.empty() && tilePalettePrepared_) {
				ImGui::TextDisabled("No tiles loaded. Check texture keys and tiles.json.");
			}

			for (const auto& item : tilePaletteItems_) {
				ImGui::PushID(item.name.c_str());
				ImTextureID textureID = item.textureID;

				if (isBrushMode_ && brushPayload_.name == item.name) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 1, 0, 0.5f)); // Green tint
				}

				if (ImGui::ImageButton(item.name.c_str(),
					textureID,
					buttonSize,
					item.uv0,
					item.uv1,
					bgCol,
					tintCol))
				{
					Framework::Logger::Get().Info(DebugType::Editor)
						<< "Clicked tile: " << item.name << std::endl;
				}

				if (isBrushMode_ && brushPayload_.name == item.name) {
					ImGui::PopStyleColor();
					// Draw border around selected
					ImVec2 p_min = ImGui::GetItemRectMin();
					ImVec2 p_max = ImGui::GetItemRectMax();
					ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(0, 255, 0, 255), 0, 0, 2.0f);
				}


				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					isBrushMode_ = false; // Cancel brush if we start dragging
					ImGui::SetDragDropPayload("TILE_DND", &item, sizeof(DraggablePrefabInfo));
					ImGui::Text("Placing: %s", item.name.c_str());
					ImGui::Image(textureID, ImVec2(32, 32), item.uv0, item.uv1);
					isDraggingPrefab_ = true;
					dragDropPayload_ = item;
					ImGui::EndDragDropSource();
				}

				// 2. Handle Right Click (New Brush Mode)
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					isBrushMode_ = true;
					brushPayload_ = item; // Copy item info to brush
					isDraggingPrefab_ = false; // Stop dragging if active

					Framework::Logger::Get().Info(DebugType::Editor)
						<< "Entered Brush Mode with: " << item.name << std::endl;
				}

				

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", item.name.c_str());

				
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

		if (!isTileEditorActive_) {
			if (wasTilePaletteOpen) {
				wasTilePaletteOpen = false;
			}
			showGrid_ = false;
			isDraggingPrefab_ = false;
		}
		
	}

	void Editor::LoadTilesFromFile() {
		using namespace Framework;

		std::ifstream fileIn("Data/tiles.json");
		if (!fileIn.is_open()) {
			Logger::Get().Error(DebugType::Editor)
				<< "Failed to open Data/tiles.json" << std::endl;
			return;
		}

		rapidjson::IStreamWrapper isw(fileIn);
		rapidjson::Document doc;
		doc.ParseStream(isw);
		fileIn.close();

		if (!doc.IsObject() || !doc.HasMember("tiles") || !doc["tiles"].IsArray()) {
			Logger::Get().Error(DebugType::Editor)
				<< "Invalid tiles.json format" << std::endl;
			return;
		}

		tileDataList_.clear();
		const rapidjson::Value& tilesArray = doc["tiles"];

		for (rapidjson::SizeType i = 0; i < tilesArray.Size(); ++i) {
			const rapidjson::Value& tileObj = tilesArray[i];

			if (!tileObj.HasMember("name") || !tileObj.HasMember("textureID")) {
				Logger::Get().Error(DebugType::Editor)
					<< "Skipping invalid tile entry at index " << i << std::endl;
				continue;
			}

			TileData tile;
			tile.name = tileObj["name"].GetString();
			tile.textureID = tileObj["textureID"].GetString();

			tileDataList_.push_back(tile);
		}

		Logger::Get().Info(DebugType::Editor)
			<< "Loaded " << tileDataList_.size() << " tiles from tiles.json" << std::endl;
	}


	void Editor::PrepareTilePaletteItems() {
		tilePaletteItems_.clear();

		for (const auto& tile : tileDataList_) {
			if (tile.textureID.empty()) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< "Skipping tile '" << tile.name << "' due to empty texture key." << std::endl;
				continue;
			}

			try {
				Framework::Logger::Get().Info(DebugType::Editor)
					<< "Attempting to get texture for tile: '" << tile.name	<< "' with key: '" << tile.textureID << "'" << std::endl;

				if (!resourceManager_) {
					Framework::Logger::Get().Error(DebugType::Editor)<< "ResourceManager pointer is null in PrepareTilePaletteItems!" << std::endl;
					return;
				}

				Texture texID = resourceManager_->get<Texture>(tile.textureID);

				if (texID != 0) {
					tilePaletteItems_.push_back({ tile.name, PrefabType::Tile, texID ,ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f) });
					Framework::Logger::Get().Info(DebugType::Editor)<< " -> Success! Added tile: '" << tile.name
						<< "' (ID: " << texID << ")" << std::endl;
				}
				else {
					Framework::Logger::Get().Error(DebugType::Editor)<< " -> Failed! Texture ID is 0 for tile: '" << tile.name
						<< "' (Key: '" << tile.textureID << "')" << std::endl;
				}
			}
			catch (const std::out_of_range&) {
				Framework::Logger::Get().Error(DebugType::Editor)<< " -> Failed! Texture key '" << tile.textureID
					<< "' not found in ResourceManager for tile: '" << tile.name << "'" << std::endl;
			}
			catch (const std::exception& e) {
				Framework::Logger::Get().Error(DebugType::Editor)<< " -> Failed! Exception getting texture for tile '" << tile.name
					<< "' (Key: '" << tile.textureID << "'): " << e.what() << std::endl;
			}
			catch (...) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< " -> Failed! Unknown error getting texture for tile: '" << tile.name<< "' (Key: '" << tile.textureID << "')" << std::endl;
			}
		}

		Framework::Logger::Get().Info(DebugType::Editor)
			<< "Prepared " << tilePaletteItems_.size() << " tile palette items." << std::endl;
	}


	void Editor::ShowTilemapEditor()
	{
		static bool wasTileEditorOpen = false; // Tracks open/close transition

		if (ImGui::Begin("Prefab Palette", &isTileEditorActive_)) {
			// --- ENTER TILE EDITOR (open event) ---
			if (!wasTileEditorOpen) {
				wasTileEditorOpen = true;

				if (engine_ && !engine_->IsEditorMode()) {
					// Save current state before switching
					SaveEditorCameraState();

					//engine_->SetMode(Framework::EngineMode::Editor);

					// Reset editor camera to known state when entering tile editor
					ResetEditorCamera();

					//Framework::Logger::Get().Info(DebugType::Editor)
					//	<< "Switched to EDITOR mode (Tilemap open)" << std::endl;
				}
			}

			ImGui::Text("Left Click to drag and drop, Right click to select item for Brush Mode");
			ImGui::Separator();

			showGrid_ = true; // Keep grid visible

			ImVec2 buttonSize(64, 64);
			ImVec4 bgCol(0, 0, 0, 0);
			ImVec4 tintCol(1, 1, 1, 1);

			// --- Eraser Tool ---
			bool isEraserActive = (isBrushMode_ && brushPayload_.type == PrefabType::Eraser);
			if (isEraserActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
			if (ImGui::Selectable("Eraser", isEraserActive)) {
				isBrushMode_ = true;
				brushPayload_ = { "Eraser", PrefabType::Eraser, 0 };
				isDraggingPrefab_ = false;
			}
			if (isEraserActive) ImGui::PopStyleColor();
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

				// Highlight if active brush
				if (isBrushMode_ && brushPayload_.name == item.name) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 1, 0, 0.5f));
				}

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

				if (isBrushMode_ && brushPayload_.name == item.name) {
					ImGui::PopStyleColor();
					ImVec2 p_min = ImGui::GetItemRectMin();
					ImVec2 p_max = ImGui::GetItemRectMax();
					ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(0, 255, 0, 255), 0, 0, 2.0f);
				}
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					isBrushMode_ = false;
					ImGui::SetDragDropPayload("PREFAB_TILE_DND", &item, sizeof(DraggablePrefabInfo));
					ImGui::Text("Placing: %s", item.name.c_str());
					ImGui::Image(textureID, ImVec2(32, 32), item.uv0, item.uv1);
					isDraggingPrefab_ = true;
					dragDropPayload_ = item;
					ImGui::EndDragDropSource();
				}

				// 2. Brush Mode (Right Click)
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					isBrushMode_ = true;
					brushPayload_ = item;
					isDraggingPrefab_ = false;
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", item.name.c_str());


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

				//engine_->SetMode(Framework::EngineMode::Play);
				//Framework::Logger::Get().Info(DebugType::Editor)
				//	<< "Switched back to PLAY mode (Tilemap closed)" << std::endl;
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
							historyManager_.Record(OpType::Create, clonedEntity, "Duplicated Entity");
						}
						if (ImGui::MenuItem("Delete")) {
							historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
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
					
					ImGui::EndMenu(); // End Prefabs Menu
				}

				ImGui::Separator();

				// Create Empty Entity
				if (ImGui::MenuItem("Create Empty")) {
					Entity newEntity = entityMgr.CreateEntity();
					SceneManager::GetInstance().AddEntityToScene(currentScene, newEntity);
					historyManager_.Record(OpType::Create, newEntity, "Create Entity");
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
			EntityStorage& ecs = *(gameManager_->GetSystems().ecs_);
			EntityManager entityMgr(ecs);
			Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);

			// Check if Entity is alive before accessing properties.
			if (!entityMgr.ValidateEntity(selectedEntity)) {
				isEntitySelected_ = false;
				selectedEntityIndex = selectedComponentIndex = selectedTagIndex = selectedScriptIndex = 0;
				ImGui::End();
				return;
			}

			// --- Name Field ---
			std::string& entityName = entityMgr.GetProperties(selectedEntity).name;

			static Entity lastSelectedEntity = 0;
			static char nameBuffer[128] = "";
			static std::string lastRecordedName;
			static bool editingName = false;

			if (selectedEntity != lastSelectedEntity) {
				strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
				nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // ensure null-termination
				lastSelectedEntity = selectedEntity;
				lastRecordedName = entityName;
				editingName = false;
			}

			// Check if got mismatch between buffer and name when not editing
			if (!editingName && lastRecordedName != entityName) {
				lastRecordedName = entityName;

				strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
			}

			if (ImGui::InputText("##Entity Name", nameBuffer, sizeof(nameBuffer)));
			if (ImGui::IsItemActivated()) editingName = true;
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				std::string newName = nameBuffer;

				if (newName != entityName) {
					std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);
					entityName = newName;
					std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

					std::stringstream ss;
					ss << "Modified name in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
				}

				editingName = false;
			}
			// --- End Name Field ---

			// --- Layer Dropdown ---
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
				std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);
				entityLayer = static_cast<CollisionLayer>(currentLayer);
				std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified layer in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
				}
			}
			// -- End Layer Dropdown ---

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
								std::string beforeJSON;

								// auto calculate aabb halfsize using tex halfsize
								if (selectedComponent == ComponentType::AABBColliderType) {
									AABBCollider newCollider;

									// If we have a texture, use its size automatically
									if (entityMgr.HasComponent(selectedEntity, ComponentType::TextureComponentType)) {
										const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);
										newCollider.HalfSize = tex.HalfSize;
									}
									else {
										newCollider.HalfSize = { 50.0f, 50.0f }; // Default fallback
									}

									entityMgr.AddComponent(selectedEntity, newCollider);
								}
								else {
									// Standard addition for other components
									entityMgr.AddComponent(selectedEntity, selectedComponent);
								}

								std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, ecs);
								std::string description = "Add " + ComponentToString(selectedComponent);
								historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedComponent);
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove Component")) {
							if (entityMgr.HasComponent(selectedEntity, selectedComponent)) {
								std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, ecs);

								entityMgr.RemoveComponent(selectedEntity, selectedComponent);

								std::string afterJSON;
								std::string description = "Remove " + ComponentToString(selectedComponent);
								historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedComponent);
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
								std::string beforeJSON = EntitySerializer::SerializeTagsToString(selectedEntity, ecs);

								entityMgr.AddComponent(selectedEntity, selectedTag);

								std::string afterJSON = EntitySerializer::SerializeTagsToString(selectedEntity, ecs);
								std::string description = "Add " + ComponentToString(selectedTag);
								if (beforeJSON != afterJSON) {
									historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedTag);
								}
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove Tag")) {
							if (entityMgr.HasComponent(selectedEntity, selectedTag)) {
								std::string beforeJSON = EntitySerializer::SerializeTagsToString(selectedEntity, ecs);

								entityMgr.RemoveComponent(selectedEntity, selectedTag);
								std::string afterJSON = EntitySerializer::SerializeTagsToString(selectedEntity, ecs);
								std::string description = "Remove " + ComponentToString(selectedTag);
								if (beforeJSON != afterJSON) {
									historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedTag);
								}
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

		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);

		ImGui::SeparatorText(ComponentToString(selectedComponent).c_str());
		switch (selectedComponent) {
		case ComponentType::PositionType: {
			Position& position = entityMgr.GetComponent<Position>(selectedEntity);

			ImGui::InputFloat2("Position", &position.Pos.x, "%.2f");

			bool nowActive = ImGui::IsItemActive();
			if (nowActive && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!nowActive && editSession.wasItemActive) {
				std::stringstream ss;
				ss << std::setprecision(2) << std::fixed;
				ss << "Set " << ComponentToString(ComponentType::PositionType) << " to (" << position.Pos.x << ", " << position.Pos.y << ") in "
						<< entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}
			
			editSession.wasItemActive = nowActive;
			break;
		}
		case ComponentType::RotationType: {
			Rotation& rotation = entityMgr.GetComponent<Rotation>(selectedEntity);
			static bool unifiedRotation = true;
			ImGui::Checkbox("Rotation (Texture + Collider)", &unifiedRotation);
			ImGui::Separator();
			if (unifiedRotation) {
				float unifiedDegrees = rotation.textureAngle * (180.0f / M_PI);
				if (ImGui::SliderFloat("Rotation (Degrees)", &unifiedDegrees, -180.0f, 180.0f, "%.1f")) {
					float newRadians = unifiedDegrees * (M_PI / 180.0f);
					rotation.textureAngle = newRadians;
					rotation.colliderAngle = newRadians;
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				if (ImGui::Button("Reset Rotation")) {
					rotation.textureAngle = 0.0f;
					rotation.colliderAngle = 0.0f;
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::Text("Current: %.1f°", unifiedDegrees);
			}
			else {
				ImGui::Text("Texture Rotation");
				float textureDegrees = rotation.textureAngle * (180.0f / M_PI);
				if (ImGui::SliderFloat("Texture (Degrees)", &textureDegrees, -180.0f, 180.0f, "%.1f")) {
					rotation.textureAngle = textureDegrees * (M_PI / 180.0f);
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::Separator();
				ImGui::Text("Collider Rotation");
				float colliderDegrees = rotation.colliderAngle * (180.0f / M_PI);
				if (ImGui::SliderFloat("Collider (Degrees)", &colliderDegrees, -180.0f, 180.0f, "%.1f")) {
					rotation.colliderAngle = colliderDegrees * (M_PI / 180.0f);
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::Separator();
				if (ImGui::Button("Reset Texture")) {
					rotation.textureAngle = 0.0f;
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset Collider")) {
					rotation.colliderAngle = 0.0f;
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset Both")) {
					rotation.textureAngle = 0.0f;
					rotation.colliderAngle = 0.0f;
					if (transformationSystem_) {
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
				}
				ImGui::Text("Texture: %.1f° | Collider: %.1f°", textureDegrees, colliderDegrees);
			}
			break;
		}
		case ComponentType::ScaleType: {
			Scale& scale = entityMgr.GetComponent<Scale>(selectedEntity);

			if (transformationSystem_) {
				static std::unordered_map<Entity, Vector2D> originalSizes;

				if (originalSizes.find(selectedEntity) == originalSizes.end()) {
					if (entityMgr.HasComponent(selectedEntity, AABBColliderType)) {
						originalSizes[selectedEntity] = entityMgr.GetComponent<AABBCollider>(selectedEntity).HalfSize;
					}
				}

				// ===========================================================
				// SCALING
				// ===========================================================
				if (entityMgr.HasComponent(selectedEntity, AABBColliderType)) {
					AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(selectedEntity);
					Vector2D ScaleFactor = { scale.axes.x, scale.axes.y };

					bool active = false;
					if (ImGui::InputFloat2("Scale (X, Y)", &ScaleFactor.x, "%.2f")) {
						scale.axes.x = ScaleFactor.x;
						scale.axes.y = ScaleFactor.y;

						if (originalSizes.count(selectedEntity)) {
							collider.HalfSize.x = originalSizes[selectedEntity].x * ScaleFactor.x;
							collider.HalfSize.y = originalSizes[selectedEntity].y * ScaleFactor.y;
						}
						transformationSystem_->UpdateEntityTransform(selectedEntity);
						active = true;
					}

					static bool uniform = false;
					ImGui::Checkbox("Uniform Scaling", &uniform);

					if (uniform) {
						float uniformValue = (ScaleFactor.x + ScaleFactor.y) * 0.5f;
						if (ImGui::SliderFloat("Uniform Scale", &uniformValue, 0.1f, 5.0f, "%.2f")) {
							scale.axes.x = uniformValue;
							scale.axes.y = uniformValue;

							if (originalSizes.count(selectedEntity)) {
								collider.HalfSize.x = originalSizes[selectedEntity].x * uniformValue;
								collider.HalfSize.y = originalSizes[selectedEntity].y * uniformValue;
							}
							transformationSystem_->UpdateEntityTransform(selectedEntity);
							active = true;
						}
					}
					if (ImGui::Button("Reset Scale")) {
						scale.axes = { 1.0f, 1.0f };
						if (originalSizes.count(selectedEntity)) {
							collider.HalfSize = originalSizes[selectedEntity];
						}
						transformationSystem_->UpdateEntityTransform(selectedEntity);
					}
					if (active && !editSession.wasItemActive) {
						editSession.Begin(selectedEntity, selectedComponent);
					}
					if (!active && editSession.wasItemActive) {
						std::stringstream ss;
						ss << "Modified Scale in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
					}
				}
				else {
					ImGui::TextDisabled("No AABBCollider attached");
				}
			}
			else {
				Framework::Logger::Get().Error(DebugType::Editor) << "Transformation system not initialised or not working" << std::endl;
			}
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

			bool activeNow = false;

			ImGui::InputFloat2("A", &triangle.a.x, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat2("B", &triangle.b.x, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat2("C", &triangle.c.x, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::ColorEdit4("RGBA", &triangle.color.r);
			if (ImGui::IsItemActive()) activeNow = true;

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(ComponentType::TriangleType) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
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

			bool activeNow = ImGui::IsItemActive();
			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified Mass in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Inverse Mass", &rb.InverseMass, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);

			editSession.wasItemActive = activeNow;
			break;
		}
		case ComponentType::AABBColliderType: {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(selectedEntity);

			ImGui::InputFloat2("HalfSize (X),(Y)", &col.HalfSize.x, "%.3f");
			bool activeNow = ImGui::IsItemActive();
			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << std::setprecision(2) << std::fixed;
				ss << "Set " << ComponentToString(ComponentType::AABBColliderType) << " to (" << col.HalfSize.x << ", " << col.HalfSize.y << ") in "
					<< entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			if (entityMgr.HasComponent(selectedEntity, ComponentType::TextureComponentType)) {
				ImGui::NewLine();
				if (ImGui::Button("Auto-Fit to Sprite")) {
					const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);

					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					col.HalfSize = tex.HalfSize;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					historyManager_.RecordModifyComponent(selectedEntity, "Auto-Fit Collider", beforeJSON, afterJSON, selectedComponent);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Automatically sets the AABB bounds to match the Texture dimensions.");
				}
			}

			editSession.wasItemActive = activeNow;
			break;
		}
		case ComponentType::ButtonType: {
			Button& buttonComp = entityMgr.GetComponent<Button>(selectedEntity);

			bool isPressedLocal = buttonComp.isPressed;
			if (ImGui::Checkbox("Is Pressed?", &isPressedLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				buttonComp.isPressed = isPressedLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			const char* items[] = { "DEFAULT", "PLAYSOUND", "RESETPLAYERPOS" };
			int currentItemIndex = static_cast<int>(buttonComp.action);

			if (ImGui::Combo("Button Action", &currentItemIndex, items, IM_ARRAYSIZE(items))) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				buttonComp.action = static_cast<ButtonAction>(currentItemIndex);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
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

			bool isRotatingLocal = rFlag.isRotating;
			if (ImGui::Checkbox("Is Rotating?", &isRotatingLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				rFlag.isRotating = isRotatingLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			};

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

			bool activeNow = false;

			ImGui::InputFloat("Trigger Delay", &trapComp.triggerDelay, 0.1f, 1.f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Current Delay", &trapComp.currentDelay, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);

			ImGui::InputFloat("Fall Speed", &trapComp.fallSpeed, 0.1f, 1.f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			ImGui::InputFloat2("Original Position", &trapComp.originalPosition.x, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Current Fall Distance", &trapComp.currentFallDistance, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
			break;
		}

		case ComponentType::TextureComponentType: {
			TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);

			//Show text but make it read-only
			ImGui::InputText("Texture Key", &tex.textureKey, ImGuiInputTextFlags_ReadOnly);

			// Drag and Drop Target for Texture Key
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload)
				{
					// Process the dropped file path
					const char* dropped_path_c_str = (const char*)payload->Data;
					fs::path dropped_path(dropped_path_c_str);

					// Validate the file extension
					std::string ext = dropped_path.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
					{
						// Update the texture key with the new file's filename
						std::string newKey = dropped_path.filename().string();
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						tex.textureKey = newKey;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}

						// Reload the texture resource
						resourceManager_->unload<Texture>(newKey);
						resourceManager_->loadTexture(newKey, dropped_path.string());

						Logger::Get().Info(DebugType::Editor) << "Texture key updated via Drag & Drop to: " << newKey << std::endl;
					}
					else
					{
						Logger::Get().Error(DebugType::Editor) << "Dropped file is not a supported image format: " << ext << std::endl;
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Manual input field for new texture key
			static std::string newTextureKey = "";
			ImGui::InputText("New Texture Key", &newTextureKey);

			//Button to update texture key
			if (ImGui::Button("Update Texture")) {
				if (!newTextureKey.empty()) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					tex.textureKey = newTextureKey;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					if (beforeJSON != afterJSON) {
						std::stringstream ss;
						ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
			}

			// Vector2D HalfSize Input
			ImGui::InputFloat2("Half Size", &tex.HalfSize.x, "%.2f");
			bool activeNow = ImGui::IsItemActive();

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
			break;
		}
		case ComponentType::AudioComponentType: {
			AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(selectedEntity);
			//Show text but make it read-only
			ImGui::InputText("Audio Key", &audioComp.soundKey, ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload)
				{
					// CORRECT way to extract the string from payload
					// payload->Data is the raw bytes, payload->DataSize is the size in bytes
					std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1); // -1 to exclude null terminator

					fs::path dropped_file(dropped_path);

					// Validate the file extension
					std::string ext = dropped_file.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					// Check for common audio formats
					if (ext == ".ogg" || ext == ".wav" || ext == ".mp3")
					{
						// Update the sound key with the new file's filename
						std::string newKey = dropped_file.filename().string();
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						audioComp.soundKey = newKey;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}

						Framework::Logger::Get().Info(DebugType::Editor)
							<< "Audio key updated via Drag & Drop to: " << newKey << std::endl;
					}
					else
					{
						Framework::Logger::Get().Error(DebugType::Editor)
							<< "Dropped file is not a supported audio format: " << ext << std::endl;
					}
				}
				ImGui::EndDragDropTarget();
			}

			static std::string newSoundKey = "";
			ImGui::InputText("New Audio Key", &newSoundKey);
			//Button to update audio key
			if (ImGui::Button("Update Audio")) {
				if (!newSoundKey.empty()) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					audioComp.soundKey = newSoundKey;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					if (beforeJSON != afterJSON) {
						std::stringstream ss;
						ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
			}

			// Looping Checkbox
			bool loopLocal = audioComp.loop;
			if (ImGui::Checkbox("Looping", &loopLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				audioComp.loop = loopLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Stream Checkbox
			bool streamLocal = audioComp.stream;
			if (ImGui::Checkbox("Stream", &streamLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				audioComp.stream = streamLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Volume Slider
			ImGui::SliderFloat("Volume", &audioComp.volume, 0.0f, 1.0f, "%.2f");
			bool activeNow = ImGui::IsItemActive();

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
			break;
		}

		case ComponentType::AnimationComponentType: {
			ImGui::TextDisabled("Properties are Hidden");
			break;
		}
		case ComponentType::TextComponentType: {
			TextComponent& textComp = entityMgr.GetComponent<TextComponent>(selectedEntity);

			// Text you want to draw
			static char textBuffer[256] = "";
			static Entity text_LastSelectedEntity;
			static std::string text_lastRecordedText;
			static bool text_editingText = false;

			if (text_LastSelectedEntity != selectedEntity) {
				text_LastSelectedEntity = selectedEntity;
				text_editingText = false;
				text_lastRecordedText = textComp.textToDraw;

				// Copy textToDraw to textBuffer if !empty() && textBuffer != textToDraw
				if (!textComp.textToDraw.empty() && std::strcmp(textBuffer, textComp.textToDraw.c_str()) != 0) {
					std::strncpy(
						textBuffer,
						textComp.textToDraw.c_str(),
						IM_ARRAYSIZE(textBuffer) - 1
					);
					textBuffer[IM_ARRAYSIZE(textBuffer) - 1] = '\0';
				}
			}

			if (!text_editingText && textComp.textToDraw != text_lastRecordedText) {
				text_lastRecordedText = textComp.textToDraw;

				if (!textComp.textToDraw.empty() && std::strcmp(textBuffer, textComp.textToDraw.c_str()) != 0) {
					std::strncpy(
						textBuffer,
						textComp.textToDraw.c_str(),
						IM_ARRAYSIZE(textBuffer) - 1
					);
					textBuffer[IM_ARRAYSIZE(textBuffer) - 1] = '\0';
				}
			}

			// Text Inputfield
			ImGui::InputText("Text", textBuffer, IM_ARRAYSIZE(textBuffer));
			if (ImGui::IsItemActivated()) text_editingText = true;
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				std::string newText = textBuffer;

				if (newText != textComp.textToDraw) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					textComp.textToDraw = newText;
					textComp.isDirty = true;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}

				text_editingText = false;
			}

			// Fontkey read-only Inputfield
			ImGui::InputText("Font Key", &textComp.fontKey, ImGuiInputTextFlags_ReadOnly);

			// Drag and Drop Target for Font Key
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload)
				{
					// Extract the dropped path
					std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1); // -1 for null terminator
					fs::path dropped_file(dropped_path);

					// Validate extension
					std::string ext = dropped_file.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".ttf" || ext == ".otf")
					{
						// Set font key to filename
						std::string newFontKey = dropped_file.filename().string();
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						textComp.fontKey = newFontKey;
						textComp.isDirty = true;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}

						// Optionally reload font resource
						if (resourceManager_)
						{
							resourceManager_->unload<Font>(newFontKey);
							resourceManager_->loadFont(newFontKey, dropped_file.string());
						}

						Logger::Get().Info(DebugType::Editor)
							<< "Font key updated via Drag & Drop to: " << newFontKey << std::endl;
					}
					else
					{
						Logger::Get().Error(DebugType::Editor)
							<< "Dropped file is not a supported font format: " << ext << std::endl;
					}
				}
				ImGui::EndDragDropTarget();
			}

			static std::string newFontKey = "";
			ImGui::InputText("New Font Key", &newFontKey);

			//Button to update Font key
			if (ImGui::Button("Update Font")) {
				if (!newFontKey.empty()) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					textComp.fontKey = newFontKey;
					textComp.isDirty = true;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					if (beforeJSON != afterJSON) {
						std::stringstream ss;
						ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
			}

			bool activeNow = false;

			// Color Input
			ImGui::Text("Color RGBA: ");
			ImGui::InputFloat("R", &textComp.color.r, 0.f, 0.f, "%.3f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("G", &textComp.color.g, 0.f, 0.f, "%.3f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("B", &textComp.color.b, 0.f, 0.f, "%.3f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("A", &textComp.color.a, 0.f, 0.f, "%.3f");
			if (ImGui::IsItemActive()) activeNow = true;

			// Font Size Input
			if (ImGui::InputFloat("Font Size", &textComp.scale, 0.f, 0.f, "%.3f")) {
				textComp.isDirty = true;
			}
			if (ImGui::IsItemActive()) activeNow = true;

			// Max Width Input
			if (ImGui::InputFloat("Max Width", &textComp.maxWidth, 0.f, 0.f, "%.3f")) {
				textComp.isDirty = true;
			}
			if (ImGui::IsItemActive()) activeNow = true;

			// Read Only HalfSize
			ImGui::InputFloat2("Half Size", &textComp.halfSize.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
			break;
		}

		case ComponentType::NativeScriptsComponentType: {
			NativeScriptsComponent& comp = entityMgr.GetComponent<NativeScriptsComponent>(selectedEntity);

			ImGui::TextDisabled("Native Scripts");
			ImGui::Separator();

			std::vector<std::string> names;
			names.emplace_back("- Select script -");

			Framework::ScriptRegistry::Get().ForEachScriptName([&](const char* n) { names.emplace_back(n); });

			std::vector<const char*> labels;
			for (std::string& s : names) labels.push_back(s.c_str());

			// Create New Script Button
			ImGui::SetNextItemWidth(160.f);
			static char newScriptName[256] = "NewScript";
			ImGui::InputText("##New Script Name", newScriptName, IM_ARRAYSIZE(newScriptName));
			ImGui::SameLine();
			if (ImGui::Button("Create New Script")) {
				NativeScriptMaker::CreateScriptPair(newScriptName, "../../owala/Native/", popupMsg);
				gameManager_->GetSystems().input_->SetKeyboardLocked(true);
				ImGui::OpenPopup("Script Result");
				ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Appearing);
			}

			// Popup after attempting to create script.
			if (ImGui::BeginPopupModal("Script Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::TextWrapped("%s", popupMsg.c_str());
				ImGui::Spacing();

				if (ImGui::Button("OK")) {
					gameManager_->GetSystems().input_->SetKeyboardLocked(false);
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			// Add Script Button
			ImGui::SetNextItemWidth(240.f);
			ImGui::Combo("##addScriptChoice", &selectedScriptIndex, labels.data(), static_cast<int>(labels.size()));
			ImGui::SameLine();
			if (ImGui::Button("Add Script") && selectedScriptIndex > 0) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				const std::string& pick = names[selectedScriptIndex];
				std::size_t idx = entityMgr.AttachScriptByName(selectedEntity, pick, true);
				if (idx == (std::size_t) -1) {
					Logger::Get().Error(DebugType::Editor) << "AttachScriptByName failed for '" << pick << "'";
				}
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Added " << pick << " Native Script to " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
				selectedScriptIndex = 0;
			}

			ImGui::Separator();

			// Per Script Section
			for (std::size_t i = 0; i < comp.scripts.size(); ++i) {
				NativeScriptsComponent::NativeScript& script = comp.scripts[i];
				ImGui::PushID(static_cast<int>(i));

				const bool isMissing = script.missing || (script.InstantiateScript == nullptr);
				const char* scriptName = script.scriptName ? script.scriptName : nullptr;
				const char* fallbackName = script.serializedTypeName.empty() ? "(Unknown)" : script.serializedTypeName.c_str();
				const char* headerLabel = isMissing ? fallbackName : (scriptName ? scriptName : fallbackName);

				if (ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen)) {

					// Warning Text if Script is missing.
					if (isMissing) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.35f, 0.35f, 1.f));
						ImGui::Text("Missing: %s", fallbackName);
						ImGui::PopStyleColor();
					}

					// Enable Checkbox
					bool enabled = script.enabled;
					if (isMissing) {
						ImGui::BeginDisabled();
						ImGui::Checkbox("Enabled", &enabled);
						ImGui::EndDisabled();
					}
					else {
						if (ImGui::Checkbox("Enabled", &enabled)) {
							std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
							script.enabled = enabled;
							std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

							if (beforeJSON != afterJSON) {
								std::stringstream ss;
								ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
								historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
							}
						}
					}

					ImGui::SameLine();

					// Execute In Edit Checkbox
					bool executeInEditMode = script.executeInEditMode;
					if (isMissing) {
						ImGui::BeginDisabled();
						ImGui::Checkbox("Execute In Edit?", &executeInEditMode);
						ImGui::EndDisabled();
					}
					else {
						if (ImGui::Checkbox("Execute In Edit?", &executeInEditMode)) {
							std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
							script.executeInEditMode = executeInEditMode;
							std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

							if (beforeJSON != afterJSON) {
								std::stringstream ss;
								ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
								historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
							}
						}
					}

					// Replace Script Button
					ImGui::SetNextItemWidth(240.f);
					ImGui::Combo("##replaceChoice", &selectedReplaceScriptIndex, labels.data(), static_cast<int>(labels.size()));
					ImGui::SameLine();
					if (ImGui::Button("Replace") && selectedReplaceScriptIndex > 0) {
						const std::string& newType = names[selectedReplaceScriptIndex];
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						std::size_t newIndex = entityMgr.AttachScriptByName(selectedEntity, newType, script.enabled);
						if (newIndex == (std::size_t)-1) {
							Logger::Get().Error(DebugType::Editor) << "Replace failed for '" << newType << "'";
						}
						else {
							if (newIndex != i) entityMgr.SwapScripts(selectedEntity, newIndex, i);
							entityMgr.RemoveScriptAt(selectedEntity, newIndex, true);
						}

						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Replaced Native Script in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
						selectedReplaceScriptIndex = 0;
					}

					// Reordering Buttons
					if (ImGui::SmallButton("Up") && i > 0) {
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						entityMgr.SwapScripts(selectedEntity, i, i - 1);
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Down") && i + 1 < comp.scripts.size()) {
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						entityMgr.SwapScripts(selectedEntity, i, i + 1);
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
					}

					// Remove Script Button
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove")) {
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						entityMgr.RemoveScriptAt(selectedEntity, i, true);
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Removed " << scriptName <<" Native Script from " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
						ImGui::PopID();
						break;
					}

					// Debug Info
					ImGui::Separator();
					ImGui::TextDisabled("Runtime:");
					ImGui::BulletText("Instance: %s", script.Instance ? "Created" : "Not created");
				}

				ImGui::PopID();
				ImGui::Separator();
			}

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
		/*	ImGui::InputFloat("Attack Timer", &ai.attackTimer, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputInt("Current Hover Node", &ai.currentHoverNode, 0, 0, ImGuiInputTextFlags_ReadOnly);*/
			ImGui::PopStyleColor(2);

			ImGui::Separator();

			bool activeNow = false;
			// --- Config ---
			ImGui::Text("Configurable Stats");
			ImGui::InputFloat("Wake-Up Radius", &ai.wakeUpRadius, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Hover Radius (Arrival)", &ai.hoverRadius, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Attack Radius", &ai.attackRadius, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Move Speed", &ai.moveSpeed, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Attack Speed", &ai.attackSpeed, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Attack Duration", &ai.attackDuration, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
		/*	ImGui::InputFloat("Attack Cooldown", &ai.attackCooldown, 0.05f, 0.1f, "%.2f");*/
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Wake-Up Time", &ai.wakeUpTime, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			// --- Hover Config ---
			ImGui::Separator();
			ImGui::Text("Hover Behavior");
			ImGui::InputFloat("Hover Height", &ai.hoverHeight, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			/*ImGui::InputFloat("Hover Patrol Radius", &ai.hoverRadius_Circle, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Hover Node Reach Dist", &ai.hoverNodeReachedDist, 0.01f, 0.05f, "%.2f");*/
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Hover Speed", &ai.hoverSpeed, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			// --- Pathfinding ---
			ImGui::Separator();
			ImGui::Text("Pathfinding");
			ImGui::InputFloat("Path Recalculate Interval", &ai.pathRecalculateInterval, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::InputFloat("Path Recalculate Timer", &ai.pathRecalculateTimer, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat2("Attack Target Pos", &ai.attackTargetPosition.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor(2);
			ImGui::Text("Current Path Waypoints: %zu", ai.currentPath.size());

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
			break;
		}

		case ComponentType::SkeletonAIComponentType: {
			SkeletonAIComponent& skelAI = entityMgr.GetComponent<SkeletonAIComponent>(selectedEntity);

			// for now just show state
			int state = static_cast<int>(skelAI.state);
			ImGui::InputInt("State (0=Idle, 1=Chase)", &state, 0, 0, ImGuiInputTextFlags_ReadOnly);

			bool activeNow = false;
			ImGui::InputFloat("Detection Radius", &skelAI.detectionRadius, 0.05f, 0.1f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Move Speed", &skelAI.moveSpeed, 0.01f, 0.05f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
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

		case ComponentType::ParticleEmitterComponentType: {
			ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(selectedEntity);
			bool isEmittingLocal = emitter.isEmitting;
			bool isParticleAnimatedLocal = emitter.isParticleAnimated;

			// Is Emitting Checkbox
			if (ImGui::Checkbox("Is Emitting?", &isEmittingLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.isEmitting = isEmittingLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Animated Particle Checkbox
			if (ImGui::Checkbox("Is Particle Animated?", &isParticleAnimatedLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.isParticleAnimated = isParticleAnimatedLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Screen space toggle
			ImGui::Checkbox("Screen Space##screenspace", &emitter.isScreenSpace);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("If enabled, particles emit in screen/UI space (fixed to screen)\n"
					"If disabled, particles emit in world space (moves with camera)");
			}

			// Texture Key
			ImGui::InputText("Texture Key##particle", &emitter.particleTextureKey, ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload) {
					const char* dropped_path_c_str = (const char*)payload->Data;
					fs::path dropped_path(dropped_path_c_str);

					std::string ext = dropped_path.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
						std::string newKey = dropped_path.filename().string();
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						emitter.particleTextureKey = newKey;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}

						// Reload the texture resource
						resourceManager_->unload<Texture>(newKey);
						resourceManager_->loadTexture(newKey, dropped_path.string());
						Logger::Get().Info(DebugType::Editor) << "Texture key updated to: " << newKey << std::endl;
					}
				}
				ImGui::EndDragDropTarget();
			}

			static std::string newParticleTextureKey = "";
			ImGui::InputText("New Texture Key##particle", &newParticleTextureKey);

			if (ImGui::Button("Update Texture##particle")) {
				if (!newParticleTextureKey.empty()) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					emitter.particleTextureKey = newParticleTextureKey;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					if (beforeJSON != afterJSON) {
						std::stringstream ss;
						ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
			}

			ImGui::Separator();
			ImGui::SeparatorText("Emission Properties");

			bool activeNow = false;
			ImGui::InputFloat("Particle Lifetime", &emitter.particleLifetime);
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Emission Per Second", &emitter.emissionRate);
			// Speed with tooltip based on space
			ImGui::InputFloat("Particle Speed", &emitter.particleSpeed);
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::IsItemHovered()) {
				if (emitter.isScreenSpace) {
					ImGui::SetTooltip("Screen space speed (0-1 range recommended)\n"
						"Typical range: 0.1 - 1.0 for screen effects");
				}
				else {
					ImGui::SetTooltip("World space speed\n"
						"Typical range: 0.5 - 5.0 for world effects");
				}
			}

			ImGui::InputFloat2("Particle Size", &emitter.particleSize.x);
			if (ImGui::IsItemActive()) activeNow = true;

			bool isAffectedByGravityLocal = emitter.isAffectedByGravity;
			if (ImGui::Checkbox("Is Affected by Gravity?", &isAffectedByGravityLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.isAffectedByGravity = isAffectedByGravityLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Effect Type
			const char* effectItems[] = { "NONE", "DRIFT", "RANDOM" };
			int currentEffectIndex = static_cast<int>(emitter.particleEffectType);

			if (ImGui::Combo("Particle Effect Type", &currentEffectIndex, effectItems, IM_ARRAYSIZE(effectItems))) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.particleEffectType = static_cast<EffectType>(currentEffectIndex);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("NONE: Particles move in straight lines");
				ImGui::Text("DRIFT: Gentle variation (recommended for rain/snow)");
				ImGui::Text("RANDOM: Chaotic erratic movement");
				ImGui::EndTooltip();
			}

			ImGui::Separator();
			ImGui::SeparatorText("Emission Style");

			// Emission Style Combo
			const char* styleItems[] = {
				"From Point",
				"From Radius",
				"From Top Viewport",
				"From Left Viewport",
				"From Right Viewport",
				"From Bottom Viewport",
				"From Line",
				"From Circle Edge"
			};

			int currentStyleIndex = static_cast<int>(emitter.emissionStyle);

			if (ImGui::Combo("Emission Style", &currentStyleIndex, styleItems, IM_ARRAYSIZE(styleItems))) {
				emitter.emissionStyle = static_cast<EmissionStyle>(currentStyleIndex);
			}

			// Emission Offset
			ImGui::Spacing();
			ImGui::Text("Emission Offset:");
			ImGui::SetNextItemWidth(200.0f);
			ImGui::DragFloat2("Offset (X, Y)", &emitter.emissionOffset.x, 0.01f, -10.0f, 10.0f, "%.2f");

			if (ImGui::IsItemHovered()) {
				if (emitter.isScreenSpace) {
					ImGui::SetTooltip("Screen space offset (0-1 normalized coordinates)\n"
						"Example: X=0, Y=-0.5 shifts down by half screen");
				}
				else {
					ImGui::SetTooltip("World space offset\n"
						"Shifts emission position by this amount in world units");
				}
			}

			// Add quick reset button
			ImGui::SameLine();
			if (ImGui::SmallButton("Reset##offset")) {
				emitter.emissionOffset = { 0.0f, 0.0f };
			}

			ImGui::Spacing();

			switch (emitter.emissionStyle) {
			case EmissionStyle::FROM_POINT:
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("Emits from screen position");
				}
				else {
					ImGui::TextDisabled("Emits from entity position");
				}
				break;

			case EmissionStyle::FROM_RADIUS:
				ImGui::InputFloat("Emission Radius##fromradius", &emitter.emissionRadius, 0.05f, 0.1f);
				if (emitter.isScreenSpace) {
					ImGui::TextWrapped("Particles spawn randomly within a radius (normalized screen coords 0-1)");
				}
				else {
					ImGui::TextWrapped("Particles spawn randomly within a radius around the entity position");
				}
				break;

			case EmissionStyle::FROM_TOP_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
				}
				ImGui::TextWrapped("Particles spawn at the top and fall down");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_LEFT_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
				}
				ImGui::TextWrapped("Particles spawn at the left edge and move right");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_RIGHT_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
				}
				ImGui::TextWrapped("Particles spawn at the right edge and move left");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_BOTTOM_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
				}
				ImGui::TextWrapped("Particles spawn at the bottom and move up");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_LINE:
				ImGui::InputFloat2("Line Start", &emitter.lineStart.x, "%.2f");
				ImGui::InputFloat2("Line End", &emitter.lineEnd.x, "%.2f");
				ImGui::TextWrapped("Particles spawn randomly along a line");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_CIRCLE_EDGE:
				ImGui::InputFloat("Circle Radius##circleradius", &emitter.circleRadius, 0.05f, 0.1f);
				if (emitter.isScreenSpace) {
					ImGui::TextWrapped("Particles spawn on circle edge (normalized screen coords 0-1)");
				}
				else {
					ImGui::TextWrapped("Particles spawn on the edge of a circle around the entity position");
				}
				break;

			default:
				break;
			}

			// Direction for viewport-based emissions
			if (emitter.emissionStyle >= EmissionStyle::FROM_TOP_VIEWPORT &&
				emitter.emissionStyle <= EmissionStyle::FROM_BOTTOM_VIEWPORT) {
				ImGui::Separator();
				ImGui::InputFloat2("Emission Direction", &emitter.emissionDirection.x, "%.2f");
				ImGui::TextWrapped("Direction vector for particle movement (normalize for consistent speed)");
			}

			ImGui::Separator();
			ImGui::SeparatorText("Advanced");
			ImGui::InputFloat2("Particle Start Velocity", &emitter.particleStartVelocity.x, "%.2f");
			ImGui::InputFloat2("Particle Acceleration", &emitter.particleAcceleration.x, "%.2f");

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified " << ComponentToString(selectedComponent) << " in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				editSession.End(selectedEntity, selectedComponent, ss.str(), historyManager_);
			}

			editSession.wasItemActive = activeNow;
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

	void Editor::HandleGameViewportInput() {

		// Handle inputs for gizmos (QTYU)
		if (gizmoSystem_) {
			// Q: Select/View Mode (No Gizmo)
			if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
				gizmoSystem_->SetGizmoMode(GizmoMode::None);
			}
			// T: Translate
			else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
				gizmoSystem_->SetGizmoMode(GizmoMode::Translate);
			}
			// Y: Rotate
			else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
				gizmoSystem_->SetGizmoMode(GizmoMode::Rotate);
			}
			// U: Scale
			else if (ImGui::IsKeyPressed(ImGuiKey_U)) {
				gizmoSystem_->SetGizmoMode(GizmoMode::Scale);
			}
		}

		// Camera Controls (WASD, Up/Down Arrows, R)
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			const float dt = Framework::Time::UnscaledDeltaTime();
			const float camSpeed = 2000.0f;

			// Move camera with WASD
			if (ImGui::IsKeyPressed(ImGuiKey_W)) cam.position.y += camSpeed * dt;
			if (ImGui::IsKeyPressed(ImGuiKey_S)) cam.position.y -= camSpeed * dt;
			if (ImGui::IsKeyPressed(ImGuiKey_A)) cam.position.x -= camSpeed * dt;
			if (ImGui::IsKeyPressed(ImGuiKey_D)) cam.position.x += camSpeed * dt;

			// Zoom in/out using Up / Down arrow keys
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
				cam.zoom *= 0.8f; // zoom in
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
				cam.zoom *= 1.2f; // zoom out
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}

			// Optional: reset zoom with R
			if (ImGui::IsKeyPressed(ImGuiKey_R)) {
				ResetEditorCamera();
			}
		}

		// Delete selected entity with Delete key
		if (isEntitySelected_ && selectedEntityIndex > 1) {
			if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
				Entity entity = selectedEntityIndex - 1;
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
				historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
				entityMgr.DestroyEntity(entity);
				if (static_cast<unsigned int>((selectedEntityIndex - 1)) == entity) {
					selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
					isEntitySelected_ = false;
				}
			}
		}
	}

	void Editor::ShowGameViewport() {
		using namespace Framework;

		ImGui::Begin("Game");

		ImVec2 viewportPanelSize = gameViewportSize_;

		if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
			Graphics* gfx = gameManager_->GetSystems().gfx_;
			SpriteRenderSystem* spriteRenderer_ = gameManager_->GetSystems().spriteRenderer_;
			TextRenderSystem* textRenderer_ = gameManager_->GetSystems().textRenderer_;
			CameraSystem* camSystem = gameManager_->GetSystems().camera_;

			if (gfx && spriteRenderer_ && camSystem) {

				// === STEP 0: UPDATE CAMERA ASPECT RATIO ===
				//camSystem->UpdateAspectRatio(viewportPanelSize.x, viewportPanelSize.y);

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
					// 1. Update Aspect Ratio based on the actual ImGui Window size
					float aspect = (viewportPanelSize.y > 0) ? (viewportPanelSize.x / viewportPanelSize.y) : 1.0f;
					camSystem->SetAspectRatio(editorCamera_, aspect);

					// 2. Manually update the camera.size. 
					Framework::Camera& cam = camSystem->GetCamera(editorCamera_);
					cam.size.x = 2.0f * cam.zoom * cam.aspectRatio;
					cam.size.y = 2.0f * cam.zoom;

					camSystem->SetActiveCamera(editorCamera_);
				}

				// === STEP 3: RENDER SPRITES ===
				spriteRenderer_->Update();
				textRenderer_->Update();

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

				// === STEP 7: DRAW OVERLAYS (GRID, DEBUG) ===
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

				if (engine_->IsEditorMode()) {

					// Draw placeholders for invisible entities (e.g. SpawnPoint)
					if (gizmoSystem_) {
						gizmoSystem_->DrawInvisPlaceholders(drawList);
					}

					if (!ImGui::IsAnyItemActive() && ImGui::IsWindowFocused()) {
						HandleGameViewportInput();
					}

					if (isEntitySelected_ && selectedEntityIndex > 1) {
						if (gizmoSystem_) {
							// draw transform gizmos on selected entity
							gizmoSystem_->DrawGizmos(drawList, selectedEntityIndex);
							// run gizmos update function to check for any gizmos dragging & apply transformations accordingly
							gizmoSystem_->Update(selectedEntityIndex, editorCamera_);
						}
					}
				}

				AISystem* aiSystem = gameManager_->GetSystems().aiSystem_;
				if (aiSystem && camSystem && aiSystem->debugVisualization) {
					Entity activeCam = engine_->IsEditorMode() && editorCamera_ != 0
						? editorCamera_
						: camSystem->GetActiveCamera();

					aiSystem->DrawDebug(camSystem, drawList, this, activeCam);
				}


				Framework::PhysicsSystem* physicsSystem = gameManager_->GetSystems().physics_;
				// Check the flag here
				if (physicsSystem && camSystem && physicsSystem->drawDebugGrid) {
					Entity activeCam = engine_->IsEditorMode() && editorCamera_ != 0
						? editorCamera_
						: camSystem->GetActiveCamera();

					// Colors for the grid
					const ImU32 inactiveColor = IM_COL32(100, 100, 100, 40); // Dim gray
					const ImU32 activeColor = IM_COL32(0, 255, 0, 90);     // Bright green

					// Get the debug data from the physics system
					std::vector<PhysicsDebugCell> cells = physicsSystem->GetDebugDrawData(camSystem, activeCam);

					// Loop and draw
					for (const auto& cell : cells) {
						// Convert world corners to screen coordinates
						ImVec2 screenMin = WorldToImGuiScreen(cell.worldMin, activeCam);
						ImVec2 screenMax = WorldToImGuiScreen(cell.worldMax, activeCam);

						if (cell.isActive) {
							drawList->AddRectFilled(screenMin, screenMax, activeColor);
						}
						else {
							drawList->AddRect(screenMin, screenMax, inactiveColor);
						}
					}
				}

				drawList->PopClipRect();
			}
		}

		ImGui::End();
	}

#pragma endregion

#pragma region LAYER
	void Editor::ShowLayerManager() {

		
	ImGui::Begin("Layer");

	auto* physics = gameManager_->GetSystems().physics_;
	if (!physics) {
		ImGui::TextDisabled("Physics system not available");
		ImGui::End();
		return;
	}

	//collision matrix section
	if (ImGui::CollapsingHeader("Collision Matrix", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextWrapped("Configure which collision layers can interact:");
		ImGui::Spacing();

		ShowCollisionMatrix(physics);
	}

	ImGui::Separator();
	ImGui::Spacing();

	//entity layer section
	if (ImGui::CollapsingHeader("Entity Layer Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (!isEntitySelected_ || selectedEntityIndex == 0) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
			ImGui::TextWrapped("Select an entity in the Hierarchy panel to configure its layers.");
			ImGui::PopStyleColor();
		}
		else {
			Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
			Entity selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);

			// Display entity name
			std::string entityName = entityMgr.GetProperties(selectedEntity).name;
			ImGui::Text("Entity: %s", entityName.c_str());
			ImGui::Spacing();
			ImGui::Separator();

			// COLLISION LAYER SECTION 
			if (ImGui::TreeNodeEx("Collision Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
				ShowEntityCollisionLayer(entityMgr, selectedEntity);
				ImGui::TreePop();
			}

			ImGui::Spacing();

			//  RENDER LAYER SECTION
			if (ImGui::TreeNodeEx("Render Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
				ShowEntityRenderLayer(entityMgr, selectedEntity);
				ImGui::TreePop();
			}
		}
	}

	ImGui::End();
	
	}

	// Helper function to show collision matrix
	void Editor::ShowCollisionMatrix(PhysicsSystem* physics) {
		int layerCount = static_cast<int>(Framework::CollisionLayer::Count);

		const float ROW_HEADER_WIDTH = 100.0f;
		const float CELL_SIZE = 30.0f;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 headerStartPos = ImGui::GetCursorScreenPos();

		ImGui::NewLine();

		// Draw Matrix Grid 
		for (int row = 0; row < layerCount; row++) {
			Framework::CollisionLayer rowLayer = static_cast<Framework::CollisionLayer>(row);
			std::string rowName = LayerToString(rowLayer);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", rowName.c_str());
			ImGui::SameLine(ROW_HEADER_WIDTH);

			for (int col = 0; col < layerCount; col++) {
				Framework::CollisionLayer colLayer = static_cast<Framework::CollisionLayer>(col);

				ImGui::PushID(row * layerCount + col);
				bool canCollide = physics->GetLayerCollision(rowLayer, colLayer);

				ImGui::SetCursorPosX(ROW_HEADER_WIDTH + (col * CELL_SIZE) +
					(CELL_SIZE - ImGui::GetFrameHeight()) * 0.5f);

				if (ImGui::Checkbox("##cb", &canCollide)) {
					physics->SetLayerCollision(rowLayer, colLayer, canCollide);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s vs %s", rowName.c_str(), LayerToString(colLayer));
				}

				ImGui::SameLine();
				ImGui::PopID();
			}
			ImGui::NewLine();
		}
	}

	// Helper function to show entity collision layer
	void Editor::ShowEntityCollisionLayer(Framework::EntityManager& entityMgr, Entity selectedEntity) {
		Framework::CollisionLayer& entityLayer = entityMgr.GetProperties(selectedEntity).layer;
		int currentLayerIndex = static_cast<int>(entityLayer);
		int layerCount = static_cast<int>(Framework::CollisionLayer::Count);

		// Create layer dropdown
		std::vector<std::string> layerLabels;
		std::vector<const char*> layerItems;

		for (int i = 0; i < layerCount; i++) {
			Framework::CollisionLayer layer = static_cast<Framework::CollisionLayer>(i);
			layerLabels.push_back(LayerToString(layer));
		}

		for (const auto& label : layerLabels) {
			layerItems.push_back(label.c_str());
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Collision Layer:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);

		if (ImGui::Combo("##CollisionLayer", &currentLayerIndex, layerItems.data(),
			static_cast<int>(layerItems.size()))) {
			entityLayer = static_cast<Framework::CollisionLayer>(currentLayerIndex);
		}

		ImGui::Spacing();
		ImGui::TextDisabled("This determines which objects can collide with this entity");

		// Show collision info
		ImGui::Spacing();
		if (ImGui::TreeNode("Collision Interactions")) {
			ImGui::Text("Can collide with:");
			ImGui::Indent();

			bool hasCollisions = false;
			auto* physics = gameManager_->GetSystems().physics_;
			for (int i = 0; i < layerCount; i++) {
				Framework::CollisionLayer otherLayer = static_cast<Framework::CollisionLayer>(i);
				if (physics->GetLayerCollision(entityLayer, otherLayer)) {
					ImGui::BulletText("%s", LayerToString(otherLayer));
					hasCollisions = true;
				}
			}

			if (!hasCollisions) {
				ImGui::TextDisabled("(No collisions enabled)");
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}
	}

	// Helper function to show entity render layer
	void Editor::ShowEntityRenderLayer(Framework::EntityManager& entityMgr, Entity selectedEntity) {
		Framework::EntityProperties& props = entityMgr.GetProperties(selectedEntity);

		// Render Layer Dropdown
		const char* renderLayerNames[] = {
			"Background (-100)",
			"Platform (50)",
			"Enemy (75)",
			"Player (100)",
			"UI (200)",
			"Custom"
		};

		int currentRenderLayerIndex = static_cast<int>(props.renderLayer);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Render Layer:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);

		if (ImGui::Combo("##RenderLayer", &currentRenderLayerIndex, renderLayerNames,
			IM_ARRAYSIZE(renderLayerNames))) {
			props.renderLayer = static_cast<Framework::RenderLayer>(currentRenderLayerIndex);
		}

		ImGui::Spacing();
		ImGui::TextDisabled("This controls when this entity is drawn relative to others");

		// Custom Render Order (only shown if Custom is selected)
		if (props.renderLayer == Framework::RenderLayer::Custom) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Custom Render Order:");
			ImGui::SetNextItemWidth(150.0f);
			ImGui::DragInt("##CustomRenderOrder", &props.customRenderOrder, 1.0f, -1000, 1000);
			ImGui::SameLine();
			if (ImGui::Button("Reset to 0")) {
				props.customRenderOrder = 0;
			}

			ImGui::Spacing();
			ImGui::TextWrapped("Lower values render first (behind). Higher values render last (in front).");
		}

		// Show render order info
		ImGui::Spacing();
		if (ImGui::TreeNode("Render Order Info")) {
			int effectiveRenderOrder = props.renderLayer == Framework::RenderLayer::Custom
				? props.customRenderOrder
				: static_cast<int>(props.renderLayer);

			ImGui::Text("Effective Render Order: %d", effectiveRenderOrder);
			ImGui::Spacing();

			ImGui::TextWrapped("Standard render order:");
			ImGui::Indent();
			ImGui::BulletText("Background: -100 (renders first)");
			ImGui::BulletText("Platform: 50");
			ImGui::BulletText("Enemy: 75");
			ImGui::BulletText("Player: 100");
			ImGui::BulletText("UI: 200 (renders last)");
			ImGui::Unindent();

			ImGui::Unindent();
			ImGui::TreePop();
		}
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

			// Calculations for button center alignment
			float regionWidth = ImGui::GetContentRegionAvail().x; // Gets available width inside the docked window
			const float buttonWidth = 40.0f;
			const float buttonHeight = 20.0f;
			const float spacing = 10.0f;
			const float totalWidth = (buttonWidth * 3) + (spacing * 2);

			// Align Center the Toolbar buttons
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
					// on pressing Stop
					gameManager_->GetSystems().levelResetter_->RunReset();
					ResetEditorCamera();
				}

				isGamePlaying = !isGamePlaying;
				isGamePaused = false;

				if (physics_->IsInStepMode()) physics_->ToggleStepMode();
				if (physics_->IsPaused()) physics_->SetPaused(false);

				if (gizmoSystem_) gizmoSystem_->SetGizmoMode(GizmoMode::None);
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

			ImGui::SameLine();

			// History Buttons
			ImVec2 historyControlBtnSize{ 40.f, 20.f };
			float historySpacing = ImGui::GetStyle().ItemSpacing.x;
			float historyTotalWidth = historyControlBtnSize.x * 2.f + historySpacing;

			float historyStartX = regionWidth * 0.75f - (historyTotalWidth * 0.5f);

			if (historyStartX < 0.f) {
				historyStartX = 0.f;
			}

			ImGui::SetCursorPosX(historyStartX);

			// Undo Button
			if (historyManager_.CanUndo()) {
				if (ImGui::Button("Undo", historyControlBtnSize)) {
					historyManager_.Undo();
				}

				if (ImGui::IsItemHovered()) {
					std::string desc = historyManager_.GetLatestUndoDescription();
					if (!desc.empty()) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(desc.c_str());
						ImGui::EndTooltip();
					}
				}
			}
			else {
				ImGui::BeginDisabled();
				ImGui::Button("Undo", historyControlBtnSize);
				ImGui::EndDisabled();
			}

			ImGui::SameLine(0.f, historySpacing);

			// Redo Button
			if (historyManager_.CanRedo()) {
				if (ImGui::Button("Redo", historyControlBtnSize)) {
					historyManager_.Redo();
				}

				if (ImGui::IsItemHovered()) {
					std::string desc = historyManager_.GetLatestRedoDescription();
					if (!desc.empty()) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(desc.c_str());
						ImGui::EndTooltip();
					}
				}
			}
			else {
				ImGui::BeginDisabled();
				ImGui::Button("Redo", historyControlBtnSize);
				ImGui::EndDisabled();
			}

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

		// Use world space grid size
		const float cellX = baseGridCellSize_;
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

		// Draw vertical lines
		for (float x = startX; x <= right; x += cellX)
		{
			Vector2D worldA = { x, bottom };
			Vector2D worldB = { x, top };
			ImVec2 a = WorldToImGuiScreen(worldA);
			ImVec2 b = WorldToImGuiScreen(worldB);
			drawList->AddLine(a, b, lineColor);
		}

		// Draw horizontal lines
		for (float y = startY; y <= top; y += cellY)
		{
			Vector2D worldA = { left, y };
			Vector2D worldB = { right, y };
			ImVec2 a = WorldToImGuiScreen(worldA);
			ImVec2 b = WorldToImGuiScreen(worldB);
			drawList->AddLine(a, b, lineColor);
		}

		// Draw world axes
		Vector2D xA = { 0.0f, bottom };
		Vector2D xB = { 0.0f, top };
		Vector2D yA = { left, 0.0f };
		Vector2D yB = { right, 0.0f };

		drawList->AddLine(WorldToImGuiScreen(xA), WorldToImGuiScreen(xB), axisXColor, 2.0f);
		drawList->AddLine(WorldToImGuiScreen(yA), WorldToImGuiScreen(yB), axisYColor, 2.0f);
	}



	Vector2D Editor::SnapToGrid(const Vector2D& worldPos, bool normalized) const
	{
		const float cell = normalized ? 0.1f : baseGridCellSize_; // world units
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

#pragma region PREFAB EDITOR
	void Editor::ShowPrefabEditor() {
		using namespace Framework;

		ImGui::Begin("Prefab Editor", &isPrefabEditorOpen_);

		// Prefab Selection Dropdown
		ImGui::Text("Select Prefab to Edit:");

		static int selectedPrefabIndex = 0;
		std::vector<std::string> prefabNames;
		std::vector<const char*> prefabItems;

		// Gather all prefab names from the ACTUAL loaded prefabs
		prefabNames.push_back("-- Select Prefab --");
		for (const auto& pair : prefabSystem_.GetAllPrefabs()) {
			prefabNames.push_back(pair.first);
		}

		for (const auto& name : prefabNames) {
			prefabItems.push_back(name.c_str());
		}

		if (ImGui::Combo("##PrefabList", &selectedPrefabIndex, prefabItems.data(), static_cast<int>(prefabItems.size()))) {
			if (selectedPrefabIndex > 0) {
				selectedPrefabForEditing_ = prefabNames[selectedPrefabIndex];

				// Determine prefab type by checking components
				EntityPrefab* prefab = prefabSystem_.GetMutablePrefab(selectedPrefabForEditing_);
				if (prefab) {
					bool hasTrap = false;
					bool hasScaling = false;

					for (const auto& compData : prefab->components) {
						if (compData.type == "TrapPlatformComponent") hasTrap = true;
					}

					if (hasTrap) selectedPrefabTypeForEditing_ = PrefabType::TrapPlatform;
					else selectedPrefabTypeForEditing_ = PrefabType::Platform;
				}
			}
			else {
				selectedPrefabForEditing_.clear();
				selectedPrefabTypeForEditing_ = PrefabType::None;
			}
		}

		ImGui::Separator();

		// Edit Prefab Properties
		if (!selectedPrefabForEditing_.empty()) {
			ImGui::Text("Editing: %s", selectedPrefabForEditing_.c_str());

			EntityPrefab* prefab = prefabSystem_.GetMutablePrefab(selectedPrefabForEditing_);
			if (!prefab) {
				ImGui::Text("Error: Prefab not found!");
				ImGui::End();
				return;
			}

			bool prefabModified = false;

			// Edit based on type
			switch (selectedPrefabTypeForEditing_) {
			case PrefabType::Platform: {
				ImGui::SeparatorText("Platform Properties");

				// Find and edit components
				for (auto& compData : prefab->components) {
					if (compData.type == "RigidBody") {
						if (compData.data.HasMember("mass")) {
							float mass = compData.data["mass"].GetFloat();
							if (ImGui::DragFloat("Mass", &mass, 0.0f, 0.0f, 1.0f)) {
								compData.data["mass"].SetFloat(mass);
								prefabModified = true;
							}
						}
					}
					else if (compData.type == "AABBCollider") {
						ImGui::Text("Collider Size:");
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							float halfX = compData.data["halfSizeX"].GetFloat();
							float halfY = compData.data["halfSizeY"].GetFloat();

							if (ImGui::DragFloat("Collider Half Size X", &halfX, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeX"].SetFloat(halfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Collider Half Size Y", &halfY, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeY"].SetFloat(halfY);
								prefabModified = true;
							}
						}
					}
					else if (compData.type == "TextureComponent") {
						ImGui::Text("Texture Size:");
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							float texHalfX = compData.data["halfSizeX"].GetFloat();
							float texHalfY = compData.data["halfSizeY"].GetFloat();

							if (ImGui::DragFloat("Texture Half Size X", &texHalfX, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeX"].SetFloat(texHalfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Texture Half Size Y", &texHalfY, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeY"].SetFloat(texHalfY);
								prefabModified = true;
							}
						}
					}
				}
				break;
			}

			case PrefabType::TrapPlatform: {
				ImGui::SeparatorText("Trap Platform Properties");

				for (auto& compData : prefab->components) {
					if (compData.type == "TrapPlatformComponent") {
						if (compData.data.HasMember("fallSpeed")) {
							float fallSpeed = compData.data["fallSpeed"].GetFloat();
							if (ImGui::DragFloat("Fall Speed", &fallSpeed, 0.0f, 0.0f, 10000.0f)) {
								compData.data["fallSpeed"].SetFloat(fallSpeed);
								prefabModified = true;
							}
						}
						if (compData.data.HasMember("triggerDelay")) {
							float triggerDelay = compData.data["triggerDelay"].GetFloat();
							if (ImGui::DragFloat("Trigger Delay", &triggerDelay, 0.0f, 0.0f, 5.0f)) {
								compData.data["triggerDelay"].SetFloat(triggerDelay);
								prefabModified = true;
							}
						}
					}
					else if (compData.type == "AABBCollider") {
						ImGui::Text("Collider Size:");
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							float halfX = compData.data["halfSizeX"].GetFloat();
							float halfY = compData.data["halfSizeY"].GetFloat();

							if (ImGui::DragFloat("Collider Half Size X", &halfX, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeX"].SetFloat(halfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Collider Half Size Y", &halfY, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeY"].SetFloat(halfY);
								prefabModified = true;
							}
						}
					}
					else if (compData.type == "TextureComponent") {
						ImGui::Text("Texture Size:");
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							float texHalfX = compData.data["halfSizeX"].GetFloat();
							float texHalfY = compData.data["halfSizeY"].GetFloat();

							if (ImGui::DragFloat("Texture Half Size X", &texHalfX, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeX"].SetFloat(texHalfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Texture Half Size Y", &texHalfY, 0.0f, 0.0f, 10000.0f)) {
								compData.data["halfSizeY"].SetFloat(texHalfY);
								prefabModified = true;
							}
						}
					}
				}
				break;
			}
			default:
				break;
			}

			if (prefabModified) {
				// Apply changes to existing entities AND ensure new entities use updated prefab
				ApplyPrefabChangesToEntities(selectedPrefabForEditing_);

				// Force palette to reload so new drag/drop uses updated prefab
				palettePrepared_ = false;
			}

			ImGui::Separator();

			// Save button
			if (ImGui::Button("Save Prefab to File")) {
				SavePrefabToFile(selectedPrefabForEditing_);

				// Reload to ensure everything is synchronized
				if (!prefabSystem_.LoadPrefabsFromFile("Data/platformPrefab.json")) {
					std::cout << "Failed to reload prefabs after save!" << std::endl;
				}
				else {
					std::cout << "Reloaded prefabs after save" << std::endl;
				}

				palettePrepared_ = false;
			}

			if (ImGui::Button("Reload from File")) {
				// Clear current prefabs
				prefabSystem_ = Framework::PrefabSystem();

				// Reload from folder
				if (!prefabSystem_.LoadPrefabsFromFolder("Data/prefabs/")) {
					Framework::Logger::Get().Error(DebugType::Editor)
						<< "Failed to reload prefabs from folder!" << std::endl;
				}
				else {
					Framework::Logger::Get().Info(DebugType::Editor)
						<< "Successfully reloaded prefabs from folder" << std::endl;
				}

				if (!selectedPrefabForEditing_.empty()) {
					ApplyPrefabChangesToEntities(selectedPrefabForEditing_);
				}

				paletteItems_.clear();
				palettePrepared_ = false;
			}
		}

		ImGui::End();
	}

	// Apply prefab changes to all entities using this prefab
	void Editor::ApplyPrefabChangesToEntities(const std::string& prefabName) {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		SceneManager& sceneMgr = SceneManager::GetInstance();

		const EntityPrefab* prefab = prefabSystem_.GetPrefab(prefabName);
		if (!prefab) return;

		// Get all entities in the current scene
		const std::vector<Entity>& entities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

		for (const Entity& entity : entities) {
			// Check if entity name matches prefab name
			if (entityMgr.GetProperties(entity).name != prefabName) continue;

			// Update components based on prefab data
			for (const auto& compData : prefab->components) {
				if (compData.type == "RigidBody") {
					if (entityMgr.HasComponent(entity, RigidBodyType)) {
						RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
						if (compData.data.HasMember("mass")) {
							rb.SetMass(compData.data["mass"].GetFloat());
						}
					}
				}
				else if (compData.type == "AABBCollider") {
					if (entityMgr.HasComponent(entity, AABBColliderType)) {
						AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							col.HalfSize.x = compData.data["halfSizeX"].GetFloat();
							col.HalfSize.y = compData.data["halfSizeY"].GetFloat();

							if (entityMgr.HasComponent(entity, TextureComponentType)) {
								TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);
								tex.HalfSize = col.HalfSize;
							}
						}
					}
				}
				else if (compData.type == "TextureComponent") {
					if (entityMgr.HasComponent(entity, TextureComponentType)) {
						TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);
						if (compData.data.HasMember("halfSizeX") && compData.data.HasMember("halfSizeY")) {
							tex.HalfSize.x = compData.data["halfSizeX"].GetFloat();
							tex.HalfSize.y = compData.data["halfSizeY"].GetFloat();
						}
					}
				}
				else if (compData.type == "TrapPlatformComponent") {
					if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
						TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
						if (compData.data.HasMember("fallSpeed")) {
							trap.fallSpeed = compData.data["fallSpeed"].GetFloat();
						}
						if (compData.data.HasMember("triggerDelay")) {
							trap.triggerDelay = compData.data["triggerDelay"].GetFloat();
						}
					}
				}
			}
		}
	}

	// Save prefab changes to JSON file
	void Editor::SavePrefabToFile(const std::string& prefabName) {
		using namespace Framework;

		// Use the PrefabSystem's save method instead
		if (prefabSystem_.SavePrefabToFile(prefabName)) {
			Logger::Get().Info(DebugType::Editor)<< "Successfully saved prefab: " << prefabName << std::endl;

			// Apply changes to existing entities
			ApplyPrefabChangesToEntities(prefabName);

			// Force palette reload
			palettePrepared_ = false;
		}
		else {
			Logger::Get().Error(DebugType::Editor)<< "Failed to save prefab: " << prefabName << std::endl;
		}
	}


	void Editor::ApplyAllPrefabChangesToCurrentScene() {
		using namespace Framework;

		EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		SceneManager& sceneMgr = SceneManager::GetInstance();
		const std::vector<Entity>& entities =
			sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

		// Get all unique prefab names used in this scene
		std::unordered_set<std::string> usedPrefabs;
		for (const Entity& entity : entities) {
			std::string prefabName = entityMgr.GetProperties(entity).name;
			if (prefabSystem_.HasPrefab(prefabName)) {
				usedPrefabs.insert(prefabName);
			}
		}

		// Apply updates for each prefab
		for (const std::string& prefabName : usedPrefabs) {
			ApplyPrefabChangesToEntities(prefabName);
		}

		Logger::Get().Info(DebugType::Editor)
			<< "Applied prefab updates to " << usedPrefabs.size()
			<< " prefab types in current scene" << std::endl;
	}

#pragma endregion

	ImVec2 Editor::WorldToImGuiScreen(const Vector2D& worldPos, Entity cameraID)
	{
		if (!cameraSystem_ || cameraID == 0)
			return ImVec2(0, 0);

		const Camera& cam = cameraSystem_->GetCamera(cameraID);

		// Convert world coordinates to screen coordinates
		float screenX = ((worldPos.x - cam.position.x) / cam.size.x + 0.5f) * gameViewportSize_.x + gameViewportPos_.x;
		float screenY = (0.5f - (worldPos.y - cam.position.y) / cam.size.y) * gameViewportSize_.y + gameViewportPos_.y;

		return ImVec2(screenX, screenY);
	}

	ImVec2 Editor::WorldToImGuiScreen(const Vector2D& worldPos)
	{
		// use overloaded function, defaulting to the editor camera
		return WorldToImGuiScreen(worldPos, editorCamera_);
	}

	ImVec2 Editor::NormalizedToImGuiScreen(const Vector2D& normalizedPos) const {
		// X-Axis (Maps -1 to 1)
		float normalizedX = (normalizedPos.x + 1.0f) / 2.0f;
		float screenX = normalizedX * gameViewportSize_.x + gameViewportPos_.x;

		// Y-Axis (Maps -1 to 1)
		float normalizedY = (normalizedPos.y + 1.0f) / 2.0f;
		// Invert Y for IMGUI screen
		float invertedY = 1.0f - normalizedY;
		float screenY = invertedY * gameViewportSize_.y + gameViewportPos_.y;

		return ImVec2(screenX, screenY);
	}


	void Editor::ShowDebugVisualizationPanel() {
		if (!ImGui::Begin("Debug Visualization")) {
			ImGui::End();
			return;
		}

		// Get the AI system from game manager
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

		ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Add some spacing
		ImGui::Text("Physics Visualization");
		ImGui::Separator();

		Framework::PhysicsSystem* physicsSystem = gameManager_->GetSystems().physics_;
		if (physicsSystem) {
			ImGui::Checkbox("Draw Physics Grid", &physicsSystem->drawDebugGrid);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Shows the broad-phase spatial grid.");
			}
		}
		else {
			ImGui::TextDisabled("Physics System not available");
		}

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

		/*if (ImGui::CollapsingHeader("Path Legend")) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Magenta Line = Current Path");
			ImGui::Text("Squares = Waypoints");
		}*/

		ImGui::End();
	}

	Entity Editor::GetObjAtMousePos() {
		Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		Vector2D mouseWorldPos = GetMouseWorldPosition();
		Entity bestCandidate = 0;

		// Get the list of entities from the active scene 
		const std::vector<Entity>& activeEntities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);
		std::vector<Entity> sortedByRenderLayer = activeEntities;

		// sort entities by descending render layer
		SortEntitiesByRenderLayer(sortedByRenderLayer, entityMgr);

		// Iterate through entities to find which one was clicked.
		for (Entity& entity : sortedByRenderLayer) {
			if (entityMgr.HasComponent(entity, Framework::ComponentType::PositionType))
			{
				Vector2D& pos = entityMgr.GetComponent<Framework::Position>(entity).Pos;
				bool isGUI = entityMgr.HasComponent(entity, GUITagType);
				Vector2D checkPos = isGUI ? GetNormalizedMousePos() : mouseWorldPos;

				Vector2D halfSize{};
				if (entityMgr.HasComponent(entity, Framework::ComponentType::TextComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextComponent>(entity).halfSize;
				}
				else if (entityMgr.HasComponent(entity, Framework::ComponentType::TextureComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextureComponent>(entity).HalfSize;
				}
				else {
					continue;
				}

				// check using halfsize.
				if (checkPos.x >= pos.x - halfSize.x && checkPos.x <= pos.x + halfSize.x &&
					checkPos.y >= pos.y - halfSize.y && checkPos.y <= pos.y + halfSize.y)
				{
					bestCandidate = entity;
					break; // Stop after finding the top-most one
				}
			}
		}

		return bestCandidate;
	}

	void Editor::SortEntitiesByRenderLayer(std::vector<Entity>& entities, Framework::EntityManager& entityMgr) {
		std::sort(entities.begin(), entities.end(),
			[&](const Entity& a, const Entity& b) {
				// Get properties for both entities
				EntityProperties propsA = entityMgr.GetProperties(a);
				EntityProperties propsB = entityMgr.GetProperties(b);
				
				// sort by custom render layer if both are custom
				if (propsA.renderLayer == RenderLayer::Custom && propsB.renderLayer == RenderLayer::Custom) {
					return propsA.customRenderOrder > propsB.customRenderOrder;
				}
				// compare custom render order with normal render layer if only one is custom
				if (propsA.renderLayer == RenderLayer::Custom) {
					return (int)propsA.customRenderOrder > (int)propsB.renderLayer;
				}
				if (propsB.renderLayer == RenderLayer::Custom) {
					return (int)propsA.renderLayer > (int)propsB.customRenderOrder;
				}
				// if neither are custom render layer, sort by renderLayer
				if (propsA.renderLayer != propsB.renderLayer) {
					return propsA.renderLayer > propsB.renderLayer;
				}
				return propsA.renderLayer > propsB.renderLayer;
			}
		);
	}

}