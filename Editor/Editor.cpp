/*********************************************************************//**
* \file   Editor.cpp
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
*
* \brief	Implementation of the Editor system for in-game level editing and
			debugging using ImGUI.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "Editor.h"
#include "Core.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "CameraSystem.h"
#include "Graphics.h"     // Include Graphics for drawing preview
#include "AssetBrowser.h"
#include "AISystem.h" // Include AISystem for debugging purposes
#include "FileUtility.h"

// Debug
#include "Logger.h"

// Serialization
#include "EntitySerializer.h"
#include "SerializationKeys.h"

// Time Management
#include "OwalaTime.h"

// Native Scripting
#include "ScriptableEntity.h"
#include "ScriptRegistry.h"
#include "NativeScriptMaker.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Internal Helpers
namespace Editor {
	using namespace Framework;

	std::string ComponentToString(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::TransformType:					return "Transform";
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
		case ComponentType::FSMComponentType:				return "FSM";
		case ComponentType::ParticleEmitterComponentType:	return "ParticleEmitter";
		case ComponentType::ParticleComponentType:			return "Particle";
		case ComponentType::SliderType:						return "Slider";
		case ComponentType::PlayerHealthComponentType:		return "PlayerHealth";
		case ComponentType::PlayerTagType:					return "PlayerTag";
		case ComponentType::PlatformTagType:				return "PlatformTag";
		case ComponentType::BackgroundTagType:				return "BackgroundTag";
		case ComponentType::EnemyTagType:					return "EnemyTag";
		case ComponentType::SceneOwnedType:					return "SceneOwned";
		case ComponentType::PersistentTagType:				return "PersistentTag";
		case ComponentType::LoadedTagType:					return "LoadedTag";
		case ComponentType::GUITagType:						return "GUITag";
		case ComponentType::SpawnPointTagType:				return "SpawnPointTag";
		case ComponentType::ViewportConstrainedTagType:		return "ViewportConstrainedTag";

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

	void EditSession::Flush() {
		using namespace Framework;

		if (!active) return;

		EntityStorage* ecs = GameManager::GetInstance().GetSystems().ecs_;
		EntityManager entityMgr(*ecs);

		if (!entityMgr.ValidateEntity(entityInEdit)) {
			*this = {};
			return;
		}

		std::stringstream ss;
		ss << "Modified " << ComponentToString(compInEdit) << " in " << entityMgr.GetProperties(entityInEdit).name << std::endl;
		End(entityInEdit, compInEdit, ss.str(), *GameManager::GetInstance().GetEditor()->GetHistoryMgr());
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
		historyManager_() {
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

		}
	}

	void Editor::RestoreEditorCameraState() {
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
			cam.position = currentEditorCameraState_.position;
			cam.zoom = currentEditorCameraState_.zoom;
			cameraSystem_->SetZoom(editorCamera_, currentEditorCameraState_.zoom);

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
	// Edtior Configuration
	// ----------------------------------------------------------------------------
	void Editor::LoadEditorConfig() {
		rapidjson::Document doc;
		if (Framework::FileIOSystem::ReadFile(FileIOKeys::EDITOR_CFG_PATH, doc)) {
			if (doc.HasMember(FileIOKeys::EDITOR_HISTORY_STACK) && doc[FileIOKeys::EDITOR_HISTORY_STACK].IsInt()) {
				historyManager_.SetUndoLimit(doc[FileIOKeys::EDITOR_HISTORY_STACK].GetInt());
			}
		}
	}

	void Editor::SaveEditorConfig() {
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		doc.AddMember(rapidjson::StringRef(FileIOKeys::EDITOR_HISTORY_STACK), historyManager_.GetUndoLimit(), alloc);
		Logger::Get().Info(DebugType::FileIOSystem) << "Saving Editor Config!" << std::endl;
		if (Framework::FileIOSystem::WriteFileJson(FileIOKeys::EDITOR_CFG_PATH, doc)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "Editor Config Saved!" << std::endl;
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
		io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

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

		// Restore source data dir from active_game.cmake if one was previously set.
		// This ensures saves are mirrored back to the source project across editor restarts.
		{
			fs::path repoRoot   = fs::current_path().parent_path().parent_path();
			fs::path cmakePath  = repoRoot / "active_game.cmake";
			if (fs::exists(cmakePath)) {
				std::ifstream cmakeFile(cmakePath);
				std::string line;
				while (std::getline(cmakeFile, line)) {
					// Parse the game name; reconstruct the absolute path from the known repo root.
					// ACTIVE_GAME_DIR is written as a CMake variable expression and cannot be
					// used directly from C++, so we derive the path here instead.
					const std::string prefix = "set(ACTIVE_GAME_NAME \"";
					auto pos = line.find(prefix);
					if (pos != std::string::npos) {
						std::string gameName = line.substr(pos + prefix.size());
						if (!gameName.empty() && gameName.back() == ')') gameName.pop_back();
						if (!gameName.empty() && gameName.back() == '"') gameName.pop_back();
						fs::path dir = repoRoot / gameName;
						if (fs::exists(dir)) {
							FileIOSystem::SetSourceDataDir((dir / "Data").string());
							Logger::Get().Info(DebugType::Editor)
								<< "Restored source data dir: " << dir.string() << "/Data" << std::endl;
						}
						break;
					}
				}
			}
		}

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
				if (type == ComponentType::LoadedTagType) continue; // Skip ActiveTag since user will not be able to manually add/remove it.
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

		//LoadTilesFromFile();
		//if (!layerManager_.LoadFromFile(FileIOKeys::LAYER_CONFIG_FILEPATH, gameManager_->GetSystems().physics_)) {
		//	Logger::Get().Info(DebugType::Editor)
		//		<< "Using default layer configuration" << std::endl;

		//	// Save defaults to file for next time
		//	layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH, gameManager_->GetSystems().physics_);
		//}

		std::string layerConfigPath = FileIOKeys::LAYER_CONFIG_FILEPATH;
		if (std::filesystem::exists(std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + layerConfigPath)) {
			layerConfigPath = std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + layerConfigPath;
			Logger::Get().Info(DebugType::Editor)
				<< "Using root folder layer config: " << layerConfigPath << std::endl;
		}

		if (!layerManager_.LoadFromFile(layerConfigPath, gameManager_->GetSystems().physics_)) {
			Logger::Get().Info(DebugType::Editor)
				<< "Using default layer configuration" << std::endl;

			// Save defaults to file for next time
			layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH, gameManager_->GetSystems().physics_);
		}

		gameManager_->GetSystems().physics_->ReloadCollisionMatrixFromFile();
		Logger::Get().Info(DebugType::Editor)
			<< "Collision matrix reloaded from configuration file" << std::endl;

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

		LoadEditorConfig();

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
		if (!isEnabled) return;

		SanityCheck();

		// SAFETY BLOCK FOR CONTEXT SWITCH (ALT TAB OR WTV) 
		if (gameManager_) {
			cameraSystem_ = gameManager_->GetSystems().camera_;
			graphicsSystem_ = gameManager_->GetSystems().gfx_;
			transformationSystem_ = gameManager_->GetSystems().transformation_;
			if (!transformationSystem_) {
				Framework::Logger::Get().Error(DebugType::Editor) << "Transformation system not available" << std::endl;
			}
			// Tell the InputSystem whether to lock gameplay input
			Framework::InputSystem* input = gameManager_->GetSystems().input_;
			input->SetCursorLocked(false);
		}

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

		/*if (!isEnabled) return;
		SanityCheck();*/

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

		bool dockspace_open = ImGui::Begin("Owala Editor Dockspace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		if (dockspace_open) {
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
				ImGui::DockBuilderDockWindow("Animation Editor", dock_bottom);
				ImGui::DockBuilderDockWindow("FSM Enemy Editor", dock_bottom);

				ImGuiID dock_bottom_left = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.2f, nullptr, &dock_bottom);
				ImGui::DockBuilderDockWindow("Project Explorer", dock_bottom_left);

				ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
				ImGui::DockBuilderDockWindow("Debug Visualization", dock_left);
				ImGui::DockBuilderDockWindow("Level Editor", dock_left);
				ImGui::DockBuilderDockWindow("Hierarchy", dock_left);


				ImGui::DockBuilderDockWindow("Game", dock_main_id);
				ImGui::DockBuilderFinish(dockspaceID);
			}
		}

		ImGui::End();

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
		if (isFSMEditorOpen_ && isDraggingFSM_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			HandleFSMDragDropPlacement();
		}
		if (isDraggingFSM_ && isMouseOverGameViewport_) { // fsm preview shld prolly be in a function
			ImDrawList* drawList = ImGui::GetForegroundDrawList();
			Vector2D worldMousePos = GetMouseWorldPosition();
			Vector2D snappedPos = SnapToGrid(worldMousePos);

			ImVec2 screenPos = WorldToImGuiScreen(snappedPos);

			// Draw preview
			ImVec2 rectMin = ImVec2(screenPos.x - fsmDragDropPayload_.halfSize.x,
				screenPos.y - fsmDragDropPayload_.halfSize.y);
			ImVec2 rectMax = ImVec2(screenPos.x + fsmDragDropPayload_.halfSize.x,
				screenPos.y + fsmDragDropPayload_.halfSize.y);

			drawList->AddImage(
				fsmDragDropPayload_.textureID,
				rectMin,
				rectMax,
				fsmDragDropPayload_.uv0,
				fsmDragDropPayload_.uv1,
				IM_COL32(255, 255, 255, 150)
			);
		}

		// Draw marquee selection rectangle in viewport (I mode only).
		if (isMarqueeMode_ && isMarqueeSelecting_) {
			ImDrawList* drawList = ImGui::GetForegroundDrawList();
			const ImVec2 rectMin(
				std::min(marqueeStartScreenPos_.x, marqueeEndScreenPos_.x),
				std::min(marqueeStartScreenPos_.y, marqueeEndScreenPos_.y));
			const ImVec2 rectMax(
				std::max(marqueeStartScreenPos_.x, marqueeEndScreenPos_.x),
				std::max(marqueeStartScreenPos_.y, marqueeEndScreenPos_.y));

			const ImVec2 clipMin = gameViewportPos_;
			const ImVec2 clipMax(gameViewportPos_.x + gameViewportSize_.x, gameViewportPos_.y + gameViewportSize_.y);
			drawList->PushClipRect(clipMin, clipMax, true);
			drawList->AddRectFilled(rectMin, rectMax, IM_COL32(80, 140, 255, 40));
			drawList->AddRect(rectMin, rectMax, IM_COL32(80, 140, 255, 220), 0.0f, 0, 1.5f);
			drawList->PopClipRect();
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
		ShowMenuBar();
		// --- Detached Panels ---
		if (settings.showPreferencesWindow) ShowPreferencesWindow();
		if (settings.showProjectSettingsWindow) ShowProjectSettingsWindow();
		buildSizeAnalyzer_.ShowUI();

		// --- Docked Panels ---
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
		if (isFSMEditorOpen_) ShowFSMEditor();

		if (settings.showGameViewport) ShowGameViewport();
		ShowDebugVisualizationPanel(); 
		if (isPrefabEditorOpen_) ShowPrefabEditor();
		ShowCreatePrefabDialog();
		// To show animation editor beside asset viewer
		if (settings.showAnimationEditor) {
			if (ImGui::Begin("Animation Editor", &settings.showAnimationEditor)) {
				ShowAnimationEditor();
			}
			ImGui::End();
		}

		// Set editor play/edit mode
		if (isTileEditorActive_ || !isGamePlaying || (gizmoSystem_ && gizmoSystem_->IsGizmosToggled())) {
			engine_->SetMode(EngineMode::Editor);
		}
		else if (isGamePlaying) {
			engine_->SetMode(EngineMode::Play);
		}

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
			// Skip tiles - they should only appear in tile palette
			if (name.find("StagePlatform") != std::string::npos ||
				name.find("StaticPlatform") != std::string::npos) {
				return;
			}
			if (textureKey.empty()) {
				Framework::Logger::Get().Info(DebugType::Editor)
					<< "Adding palette item '" << name << "' with no texture (placeholder)." << std::endl;
				// Add with textureID 0 — DrawDragDropPreview and the palette UI
				// will need to handle this, but the item will at least appear.
				paletteItems_.push_back({ name, type, 0, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f) });
				return;
			}
			try {
				// Log before attempting to get the texture
				//Framework::Logger::Get().Info(DebugType::Editor) << "Attempting to get texture for palette item: '" << name << "' with key: '" << textureKey << "'" << std::endl;

				// Ensure resourceManager_ pointer is valid
				if (!resourceManager_) {
					Framework::Logger::Get().Error(DebugType::Editor) << "ResourceManager pointer is null in AddPaletteItem!" << std::endl;
					return;
				}

				// Get texture ID from ResourceManager
				Texture texID = resourceManager_->get<Texture>(textureKey);

				if (texID != 0) {
					paletteItems_.push_back({ name, type, texID , ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f) });
					//Framework::Logger::Get().Info(DebugType::Editor) << " -> Success! Added palette item: '" << name << "' (ID: " << texID << ")" << std::endl;
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


		
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		
		PlaceObjectInWorld(dragDropPayload_, snappedPos);

		isDraggingPrefab_ = false;

	}


	void Editor::DrawDragDropPreview(ImDrawList* drawList)
	{

		//Check Visibility
		bool showPreview = (isDraggingPrefab_ || isBrushMode_) && isMouseOverGameViewport_;
		if (!showPreview) return;

		// Get Payload
		const DraggablePrefabInfo& currentPayload = dragDropPayload_;//isDraggingPrefab_ ? dragDropPayload_ : brushPayload_;
		if (currentPayload.type == PrefabType::None || currentPayload.type == PrefabType::Eraser) return;

		
		static int debugCounter = 0;
		if (debugCounter++ % 60 == 0) { // Log once per second at 60fps
			Framework::Logger::Get().Info(DebugType::Editor)
				<< "PREVIEW: " << currentPayload.name
				<< " | TexID: " << currentPayload.textureID
				<< " | UV0: (" << currentPayload.uv0.x << "," << currentPayload.uv0.y << ")"
				<< " | UV1: (" << currentPayload.uv1.x << "," << currentPayload.uv1.y << ")" << std::endl;
		}


		

		if (!cameraSystem_ || editorCamera_ == 0) return;

		//  Calculate Position 
		Vector2D worldMousePos = GetMouseWorldPosition();
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		// Update internal state vars for logic elsewhere
		if (isDraggingPrefab_) dragDropPreviewPos_ = snappedPos;
		else brushPreviewPos_ = snappedPos;

		// Calculate Screen Rect
		Vector2D halfSize = { baseGridCellSize_/2, baseGridCellSize_/2 };

		Vector2D worldMin = { snappedPos.x - halfSize.x, snappedPos.y - halfSize.y };
		Vector2D worldMax = { snappedPos.x + halfSize.x, snappedPos.y + halfSize.y };

		ImVec2 p1 = WorldToImGuiScreen(worldMin);
		ImVec2 p2 = WorldToImGuiScreen(worldMax);

		// Normalize Min/Max to prevent negative width/height
		ImVec2 rectMin = ImVec2(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
		ImVec2 rectMax = ImVec2(std::max(p1.x, p2.x), std::max(p1.y, p2.y));

		// Draw with proper clipping
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


			if (currentPayload.textureID != 0) {
				// Cast texture ID properly for ImGui
		

				// Draw the image
				drawList->AddImage(
					texID,
					rectMin,
					rectMax,
					currentPayload.uv0,
					currentPayload.uv1,
					tint
				);
			}
			else {
				// No texture — draw a semi-transparent colored rectangle as preview
				ImU32 placeholderColor = isBrushMode_
					? IM_COL32(80, 80, 160, 200)
					: IM_COL32(80, 80, 160, 128);
				drawList->AddRectFilled(rectMin, rectMax, placeholderColor);
				drawList->AddRect(rectMin, rectMax, IM_COL32(150, 150, 255, 200), 0.0f, 0, 1.5f);
			}

			//// Draw the image
			//drawList->AddImage(
			//	texID,
			//	rectMin,
			//	rectMax,
			//	currentPayload.uv0,
			//	currentPayload.uv1,
			//	tint
			//);

		}
		
	}

	Vector2D Editor::GetPrefabHalfSize(const std::string& name, PrefabType type)
	{
		// Default size if nothing is found (e.g. 1 grid cell)
		Vector2D defaultSize = { baseGridCellSize_, baseGridCellSize_ };

		if (type == PrefabType::Tile) {
			// Tiles usually fit exactly in the grid
			return defaultSize;
		}

		const EntityPrefab* prefab = prefabSystem_.GetPrefab(name);
		if (!prefab) return defaultSize;

		// find AABBCollider data
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

		// TextureComponent data if no collider
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
		// Force Snap
		Vector2D snappedPos = SnapToGrid(position);

		Framework::EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		Framework::EntityManager entityMgr(*ecs);
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();

		// Overlap Check
		const float epsilon = 0.05f;
		const std::vector<Entity>& entities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

		for (const auto& entity : entities) {
			if (entityMgr.HasComponent(entity, Framework::TransformType)) {
				const Vector2D& pos = entityMgr.GetComponent<Framework::Transform>(entity).position;

				// Check intersection
				if (std::abs(pos.x - snappedPos.x) < epsilon &&
					std::abs(pos.y - snappedPos.y) < epsilon) {

					// Allow replacing if in Eraser mode, otherwise BLOCK placement
					if (info.type != PrefabType::Eraser) {
						return; 
					}
				}
			}
		}

		// Create Entity
		Entity newEntity = 0;

		if (info.type == PrefabType::Tile) {
			// Tiles created from prefabs too
			newEntity = prefabSystem_.CreateEntityFromPrefab(*ecs, info.name, snappedPos);
		}
		else if (info.type != PrefabType::Eraser) {
			newEntity = prefabSystem_.CreateEntityFromPrefab(*ecs, info.name, snappedPos);
		}

		if (newEntity != 0) {
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEntity);

			if (isBrushMode_) {
				historyManager_.Record(OpType::Create, newEntity, "Brush Paint");
			}
			else {
				historyManager_.Record(OpType::Create, newEntity, "Placed Prefab/Tile");
			}
		}
	}

	void Editor::HandleBrushInput()
	{
		
		if (!isBrushMode_ || isDraggingPrefab_) return;

		//Right Click in viewport or ESC to leave brushmode
		Framework::InputSystem* input = gameManager_->GetSystems().input_;
		if (input->MousePressed(Framework::InputSystem::MouseButton::Right) ||
			input->KeyPressed(Framework::InputSystem::Key::Escape) || !isTileEditorActive_){

			if (historyManager_.IsGroupActive()) {
				historyManager_.EndGroup();
			}

			isBrushMode_ = false;
			lastPlacedPos_ = { -99999.0f, -99999.0f };
			return;
		}

		//Painting Logic (Left Mouse Held)
		if (input->MouseDown(Framework::InputSystem::MouseButton::Left) && isMouseOverGameViewport_) {
			if (!historyManager_.IsGroupActive()) {
				historyManager_.BeginGroup();
			}

			Vector2D worldMousePos = GetMouseWorldPosition();
			Vector2D snappedPos = SnapToGrid(worldMousePos);

			// check if we already placed at this exact grid spot this stroke
			if (snappedPos.x != lastPlacedPos_.x || snappedPos.y != lastPlacedPos_.y) {

				if (brushPayload_.type == PrefabType::Eraser) {
					// Set payload for existing eraser logic 
					dragDropPayload_ = brushPayload_;
					HandleTileErasing();
				}
				else {
					PlaceObjectInWorld(brushPayload_, snappedPos);
				}

				lastPlacedPos_ = snappedPos;
			}
		}
		else if (input->MouseReleased(Framework::InputSystem::MouseButton::Left)) {
			// Reset last placed when mouse is let go 
			lastPlacedPos_ = { -99999.0f, -99999.0f };

			if (historyManager_.IsGroupActive()) {
				historyManager_.EndGroup();
			}
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
			if (entityMgr.HasComponent(entity, Framework::TransformType)) {
				const Vector2D& pos = entityMgr.GetComponent<Framework::Transform>(entity).position;
				if (std::abs(pos.x - snappedPos.x) < eraseTolerance && std::abs(pos.y - snappedPos.y) < eraseTolerance) {
					entityToErase = entity;
					break;
				}
			}
		}

		if (entityToErase != 0) {
			historyManager_.Record(OpType::Delete, entityToErase, "Brush Erase");

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

		if (!isMouseOverGameViewport_) {
			return;
		}

		if (!isMarqueeMode_) {
			// In non-marquee mode, keep legacy click selection only.
			if (!input->MousePressed(Framework::InputSystem::MouseButton::Left)) {
				return;
			}
			// Allow normal click selection when gizmo mode is None.
			const bool gizmoBlockingClick = gizmoSystem_ && gizmoSystem_->IsGizmosToggled() && gizmoSystem_->IsGizmosHover();
			if (!isDraggingPrefab_ && gizmoSystem_ && !gizmoBlockingClick
				&& ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				editSession.Flush();
				Entity invisEntityClicked = gizmoSystem_->IsInvisEntityClicked();
				if (invisEntityClicked > 0) {
					selectedEntityIndex = invisEntityClicked + 1;
					selectedEntities_.clear();
					selectedEntities_.push_back(invisEntityClicked);
					lastClickedEntity_ = invisEntityClicked;
					isEntitySelected_ = true;
					selectedComponentIndex = 0;
					selectedTagIndex = 0;
				}
				else {
					Entity bestCandidate = GetObjAtMousePos();
					if (bestCandidate > 0) {
						selectedEntityIndex = bestCandidate + 1;
						selectedEntities_.clear();
						selectedEntities_.push_back(bestCandidate);
						lastClickedEntity_ = bestCandidate;
						isEntitySelected_ = true;
						selectedComponentIndex = 0;
						selectedTagIndex = 0;
					}
					else {
						selectedEntityIndex = 0;
						selectedEntities_.clear();
						lastClickedEntity_ = 0;
						isEntitySelected_ = false;
						selectedComponentIndex = 0;
						selectedTagIndex = 0;
					}
				}
			}
			return;
		}

		// Marquee mode only (I): start marquee on mouse press.
		if (input->MousePressed(Framework::InputSystem::MouseButton::Left) && !isDraggingPrefab_) {
			isMarqueeSelecting_ = true;
			marqueeStartScreenPos_ = ImGui::GetMousePos();
			marqueeEndScreenPos_ = marqueeStartScreenPos_;
		}

		// Update marquee while dragging.
		if (isMarqueeSelecting_ && input->MouseDown(Framework::InputSystem::MouseButton::Left)) {
			marqueeEndScreenPos_ = ImGui::GetMousePos();
		}

		// Finalize on release: click-select (small drag) or box-select (large drag).
		if (isMarqueeSelecting_ && input->MouseReleased(Framework::InputSystem::MouseButton::Left)) {
			isMarqueeSelecting_ = false;
			marqueeEndScreenPos_ = ImGui::GetMousePos();

			const float dx = marqueeEndScreenPos_.x - marqueeStartScreenPos_.x;
			const float dy = marqueeEndScreenPos_.y - marqueeStartScreenPos_.y;
			const float dragDistSq = dx * dx + dy * dy;
			constexpr float clickThresholdSq = 9.0f; // 3px threshold

			// Flush any pending edits BEFORE changing selection
			editSession.Flush();

			// Click selection path (existing behavior).
			if (dragDistSq <= clickThresholdSq) {
				// check if clicked on invisible entity
				Entity invisEntityClicked = gizmoSystem_ ? gizmoSystem_->IsInvisEntityClicked() : 0;

				// an invisible entity was clicked
				if (invisEntityClicked > 0) {
					selectedEntityIndex = invisEntityClicked + 1;
					selectedEntities_.clear();
					selectedEntities_.push_back(invisEntityClicked);
					lastClickedEntity_ = invisEntityClicked;
					isEntitySelected_ = true;
					selectedComponentIndex = 0;
					selectedTagIndex = 0;
				}
				else {
					Entity bestCandidate = GetObjAtMousePos();
					if (bestCandidate > 0) {
						selectedEntityIndex = bestCandidate + 1;
						selectedEntities_.clear();
						selectedEntities_.push_back(bestCandidate);
						lastClickedEntity_ = bestCandidate;
						isEntitySelected_ = true;
						selectedComponentIndex = 0;
						selectedTagIndex = 0;
					}
					else {
						selectedEntityIndex = 0;
						selectedEntities_.clear();
						lastClickedEntity_ = 0;
						isEntitySelected_ = false;
						selectedComponentIndex = 0;
						selectedTagIndex = 0;
					}
				}
				return;
			}

			// Marquee selection path.
			const float minX = std::min(marqueeStartScreenPos_.x, marqueeEndScreenPos_.x);
			const float maxX = std::max(marqueeStartScreenPos_.x, marqueeEndScreenPos_.x);
			const float minY = std::min(marqueeStartScreenPos_.y, marqueeEndScreenPos_.y);
			const float maxY = std::max(marqueeStartScreenPos_.y, marqueeEndScreenPos_.y);

			Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
			Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
			const std::vector<Entity>& activeEntities = sceneMgr.GetEntitiesInScene(sceneMgr.GetActiveScene().name);

			selectedEntities_.clear();

			for (const Entity entity : activeEntities) {
				if (!entityMgr.ValidateEntity(entity) || !entityMgr.HasComponent(entity, Framework::ComponentType::TransformType)) {
					continue;
				}
				if (entityMgr.HasComponent(entity, GUITagType)) {
					continue; // marquee in game viewport targets world entities
				}

				Vector2D halfSize{};
				if (entityMgr.HasComponent(entity, Framework::ComponentType::TextComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextComponent>(entity).halfSize;
				}
				else if (entityMgr.HasComponent(entity, Framework::ComponentType::TextureComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextureComponent>(entity).HalfSize;
				}
				else if (entityMgr.HasComponent(entity, Framework::ComponentType::AABBColliderType)) {
					halfSize = entityMgr.GetComponent<Framework::AABBCollider>(entity).HalfSize;
				}
				else {
					continue;
				}

				const Transform transformComp = entityMgr.GetComponent<Framework::Transform>(entity);
				const Vector2D scaledHalfSize = { halfSize.x * transformComp.scale.x, halfSize.y * transformComp.scale.y };

				// Entity AABB in screen space.
				const ImVec2 center = WorldToImGuiScreen(transformComp.position);
				const Camera& cam = cameraSystem_->GetCamera(editorCamera_);
				const float pxPerWorldX = (cam.size.x != 0.0f) ? (gameViewportSize_.x / cam.size.x) : 0.0f;
				const float pxPerWorldY = (cam.size.y != 0.0f) ? (gameViewportSize_.y / cam.size.y) : 0.0f;

				const float entityMinX = center.x - scaledHalfSize.x * pxPerWorldX;
				const float entityMaxX = center.x + scaledHalfSize.x * pxPerWorldX;
				const float entityMinY = center.y - scaledHalfSize.y * pxPerWorldY;
				const float entityMaxY = center.y + scaledHalfSize.y * pxPerWorldY;

				// Intersects marquee rectangle.
				const bool intersects = !(entityMaxX < minX || entityMinX > maxX || entityMaxY < minY || entityMinY > maxY);
				if (intersects) {
					selectedEntities_.push_back(entity);
				}
			}

			if (!selectedEntities_.empty()) {
				selectedEntityIndex = selectedEntities_[0] + 1;
				lastClickedEntity_ = selectedEntities_[0];
				isEntitySelected_ = true;
			}
			else {
				selectedEntityIndex = 0;
				lastClickedEntity_ = 0;
				isEntitySelected_ = false;
			}
			selectedComponentIndex = 0;
			selectedTagIndex = 0;
		}
	}

	void Editor::HandleDeletion() {
		// Check if an entity is selected and if the Delete key was just pressed
		if (isEntitySelected_ && gameManager_->GetSystems().input_->KeyPressed(Framework::InputSystem::Key::Delete)) {
			EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

			// Delete all selected entities
			if (!selectedEntities_.empty()) {
				for (Entity entity : selectedEntities_) {
					if (entityMgr.ValidateEntity(entity)) {
						historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
						entityMgr.DestroyEntity(entity);
					}
				}
				selectedEntities_.clear();
				selectedEntityIndex = 0;
				isEntitySelected_ = false;
			}
			// Legacy support for old single selection
			else if (selectedEntityIndex >= 1) {
				const SceneAsset currentScene = SceneManager::GetInstance().GetActiveScene();
				if (selectedEntityIndex < static_cast<int>(SceneManager::GetInstance().GetEntitiesInScene(currentScene.name).size() + 1)) {
					Entity entity = selectedEntityIndex - 1;
					historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
					entityMgr.DestroyEntity(entity);
					selectedEntityIndex = 0;
					isEntitySelected_ = false;
				}
			}
		}
	}

#pragma region COPY AND PASTING APIs

	void Editor::CopyEntityToClipboard(const Entity entity) {
		Framework::EntityStorage* ecs = &GameManager::GetInstance().GetECS();
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		rapidjson::Value entityData(rapidjson::kObjectType);
		EntitySerializer::SerializeEntity(entity, *ecs, entityData, alloc);
		doc.CopyFrom(entityData, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
		entitiesClipboard_.clear();
		entitiesClipboard_.push_back(buffer.GetString());
	}

	void Editor::CopyEntitiesToClipboard(const std::vector<Entity>& entities) {
		entitiesClipboard_.clear();
		Framework::EntityStorage* ecs = &GameManager::GetInstance().GetECS();
		EntityManager entityMgr(*ecs);

		for (const Entity entity : entities) {
			if (!entityMgr.ValidateEntity(entity)) {
				continue;
			}
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

			rapidjson::Value entityData(rapidjson::kObjectType);
			EntitySerializer::SerializeEntity(entity, *ecs, entityData, alloc);
			doc.CopyFrom(entityData, alloc);

			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);
			entitiesClipboard_.push_back(buffer.GetString());
		}
	}

	void Editor::PasteEntityFromClipboard() {
		// Check if there is an entity in the clipboard
		if (entitiesClipboard_.empty()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "No entity in clipboard to paste!" << std::endl;
			return;
		}

		Framework::EntityStorage* ecs = &GameManager::GetInstance().GetECS();
		rapidjson::Document doc;

		doc.Parse(entitiesClipboard_[0].c_str());
		if (doc.HasParseError()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to parse entity data from clipboard!" << std::endl;
			return;
		}

		Entity pastedEntity = EntitySerializer::DeserializeEntity(*ecs, doc);
		EntityManager entityMgr(*ecs);
		entityMgr.GetProperties(pastedEntity).uuid = EntityManager::GenerateUUID();

		Framework::SceneManager::GetInstance().AddEntityToScene(Framework::SceneManager::GetInstance().GetActiveScene(), pastedEntity);
		historyManager_.Record(OpType::Create, pastedEntity, "Pasted Entity");
	}

	void Editor::PasteEntitiesFromClipboard() {
		if (entitiesClipboard_.empty()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "No entities in clipboard to paste!" << std::endl;
			return;
		}

		Framework::EntityStorage* ecs = &GameManager::GetInstance().GetECS();
		const SceneAsset& activeScene = Framework::SceneManager::GetInstance().GetActiveScene();
		
		std::vector<Entity> pastedEntities;

		for (const std::string& entityJson : entitiesClipboard_) {
			rapidjson::Document doc;
			doc.Parse(entityJson.c_str());
			
			if (doc.HasParseError()) {
				Framework::Logger::Get().Error(DebugType::Editor) << "Failed to parse entity data from clipboard!" << std::endl;
				continue;
			}

			Entity pastedEntity = EntitySerializer::DeserializeEntity(*ecs, doc);
			EntityManager entityMgr(*ecs);
			entityMgr.GetProperties(pastedEntity).uuid = EntityManager::GenerateUUID();

			Framework::SceneManager::GetInstance().AddEntityToScene(activeScene, pastedEntity);
			pastedEntities.push_back(pastedEntity);
		}

		if (!pastedEntities.empty()) {
			historyManager_.Record(OpType::Create, pastedEntities[0],
				"Pasted " + std::to_string(pastedEntities.size()) + " Entities");
		}
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
			double gameSysMs = 0.0;
			float gameSysPct = 0.0f;

			for (const SystemTime& timing : profilerDisplayData_) {
				if (timing.name == "Game System") {
					gameSysMs += timing.milliseconds;
					gameSysPct += timing.percentage;
					continue;
				}
				ImGui::Spacing();
				ImGui::Text("%s (%.2f ms)", timing.name.c_str(), timing.milliseconds);
				char label[128];
				sprintf(label, "%.1f%%", timing.percentage);
				ImGui::ProgressBar(timing.percentage / 100.0f, ImVec2(-1.0f, 0.0f), label);
				ImGui::Spacing();
				ImGui::Separator();
			}

			// Render grouped "Game Systems" bar
			if (gameSysPct > 0.0f) {
				ImGui::Spacing();
				ImGui::Text("Game Systems (%.2f ms)", gameSysMs);
				char label[128];
				sprintf(label, "%.1f%%", gameSysPct);
				ImGui::ProgressBar(gameSysPct / 100.0f, ImVec2(-1.0f, 0.0f), label);
				ImGui::Spacing();
				ImGui::Separator();
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
				if (!IsGamePlaying()) {
					historyManager_.Record(OpType::Create, newEntity, "Create Entity");
				}
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

		if (ImGui::Button("Open Animation Editor")) {
			settings.showAnimationEditor = true;
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

		if (ImGui::Button("Open FSM Editor")) {
			isFSMEditorOpen_ = true;
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
		gameManager_->OnEndPlay();
		std::string fileName = FileIOSystem::SanitizeFileName(currentScene.name) + ".json";
		std::string filePath = FileIOKeys::SCENES_FILEPATH_PREFIX + fileName;

		// If its a newly created scene, create new filepath for it based on scene name.
		if (currentScene.path.empty()) {
			sceneMgr.UpdateScenePath(currentScene.sceneIndex, filePath);
		}

		// Serialize Scene name and entities to its own json file in Data folder
		EntitySerializer::SerializeSceneData(entitiesLoaded, *(gameManager_->GetSystems().ecs_), filePath, currentScene.name);

		// Save scene asset to json file that stores all scene assets
		FileIOSystem::SaveSceneAsset(currentScene);
	}

	void Editor::SaveAndLoadSection() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		const SceneAsset& currentScene = sceneMgr.GetActiveScene();
		bool isCurrentSceneValid = currentScene.IsValid();

		ImGui::SeparatorText("Scene Management");

		// --- Save (Overwrite/Quick Save. No changes to scene name or file path) ---
		if (!isCurrentSceneValid || IsGamePlaying()) ImGui::BeginDisabled();
		if (ImGui::Button("Save")) {
			SaveScene();
		}
		if (!isCurrentSceneValid || IsGamePlaying()) ImGui::EndDisabled();

		// --- Create New Scene --- 
		ImGui::SameLine();
		if (IsGamePlaying()) ImGui::BeginDisabled();
		if (ImGui::Button("New Scene")) {
			ImGui::OpenPopup("Create New Scene");
		}
		if (IsGamePlaying()) ImGui::EndDisabled();

		// --- Unload Current Scene ---
		ImGui::SameLine();
		ImU32 buttonColor = IM_COL32(150, 50, 50, 255);
		ImU32 buttonHoverColor = IM_COL32(200, 50, 50, 255);
		ImU32 buttonActiveColor = IM_COL32(215, 50, 50, 255);

		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);

		if (!isCurrentSceneValid || IsGamePlaying()) ImGui::BeginDisabled();
		if (ImGui::Button("Unload Scene")) {
			ImGui::OpenPopup("Save");
		}
		ImGui::PopStyleColor(3);
		if (!isCurrentSceneValid || IsGamePlaying()) ImGui::EndDisabled();

		// Save Scene Modal (pop up before unloading current scene)
		if (ImGui::BeginPopupModal("Save", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Save the current scene before unloading?");

			if (ImGui::Button("Confirm", ImVec2(120, 0)))
			{
				SaveScene();
				ImGui::CloseCurrentPopup();
				sceneMgr.UnloadScene(sceneMgr.GetActiveScene());
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
				sceneMgr.UnloadScene(sceneMgr.GetActiveScene());;
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
				// skip persistent scene
				if (scene.name == "Persistent") {
					continue;
				}

				bool isCurrentScene = (id == sceneMgr.GetActiveScene().sceneIndex);

				if (isCurrentScene || !scene.IsValid() || engine_->IsPlayMode()) {
					ImGui::BeginDisabled();
				}

				if (ImGui::Button(scene.name.c_str())) {
					Logger::Get().Info(DebugType::Editor) << "Attempting to load scene: " << scene.name << std::endl;
					if (!isCurrentScene) {
						isGamePlaying = false;
						gameManager_->OnEndPlay();
						//ResetEditorCamera();
						if (!prefabSystem_.LoadPrefabsFromFolder("Data/Prefabs/")) {
							Framework::Logger::Get().Error(DebugType::Editor)
								<< "Failed to reload prefabs before scene load!" << std::endl;
						}
						else {
							Framework::Logger::Get().Info(DebugType::Editor)
								<< "Successfully reloaded prefabs before scene load" << std::endl;
						}
						sceneMgr.LoadScene(id);

						editorCamera_ = 0;
						if (cameraSystem_) {
							// Create a new independent camera entity
							editorCamera_ = cameraSystem_->CreateCamera({ 0.0f, 0.0f }, 0, false);

							// Re-apply default editor settings
							Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);
							cam.size = { 20.0f, 20.0f };
							cam.zoom = 1.0f;

							// Set Zoom/Aspect Ratio
							cameraSystem_->SetZoom(editorCamera_, 540.0f);
							float aspect = (gameViewportSize_.y > 0) ? gameViewportSize_.x / gameViewportSize_.y : 16.0f / 9.0f;
							cameraSystem_->SetAspectRatio(editorCamera_, aspect);

							// Update state trackers
							defaultEditorCameraState_.position = { 0.0f, 0.0f };
							defaultEditorCameraState_.zoom = 1.0f;
							currentEditorCameraState_ = defaultEditorCameraState_;

							// Log for verification
							Framework::Logger::Get().Info(DebugType::Editor) << "Re-created Editor Camera with ID: " << editorCamera_ << std::endl;
						}

						ApplyAllPrefabChangesToCurrentScene();

						palettePrepared_ = false;
						tilePalettePrepared_ = false;

						// reset entity selection
						selectedEntityIndex = 0;
						isEntitySelected_ = false;
					}
				}

				if (isCurrentScene || !scene.IsValid() || engine_->IsPlayMode()) {
					ImGui::EndDisabled();
				}
			}
		}
	}

#pragma endregion

#pragma region TILEMAP EDITOR PANEL

	// TILE PALETTE WINDOW
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
			bool isEraserActive = (isBrushMode_ && brushPayload_.type == PrefabType::Eraser);
			if (isEraserActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

			if (ImGui::Selectable("Eraser", isEraserActive)) {
				if (isEraserActive) {
					// Toggle OFF
					isBrushMode_ = false;
					brushPayload_ = {};
					brushPayload_.type = PrefabType::None;

					if (dragDropPayload_.type == PrefabType::Eraser) {
						dragDropPayload_ = {};
						dragDropPayload_.type = PrefabType::None;
					}
					isDraggingPrefab_ = false;
				}
				else {
					// Toggle ON
					isBrushMode_ = true;
					brushPayload_ = { "Eraser", PrefabType::Eraser, 0 };
					isDraggingPrefab_ = false;
					dragDropPayload_ = {};
					dragDropPayload_.type = PrefabType::None;
				}
			}

			if (isEraserActive) ImGui::PopStyleColor();
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

				// Right Click (Brush Mode)
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

	/*void Editor::LoadTilesFromFile() {
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
	}*/


	void Editor::PrepareTilePaletteItems() {
		tilePaletteItems_.clear();

		auto AddTileItem = [&](const std::string& name, const std::string& textureKey) {
			if (name.find("StagePlatform") == std::string::npos &&
				name.find("StaticPlatform") == std::string::npos) {
				return;
			}

			if (textureKey.empty()) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< "Skipping tile '" << name << "' due to empty texture key." << std::endl;
				return;
			}

			try {
				Framework::Logger::Get().Info(DebugType::Editor)
					<< "Attempting to get texture for tile: '" << name
					<< "' with key: '" << textureKey << "'" << std::endl;

				if (!resourceManager_) {
					Framework::Logger::Get().Error(DebugType::Editor)<< "ResourceManager pointer is null in PrepareTilePaletteItems!" << std::endl;
					return;
				}

				Texture texID = resourceManager_->get<Texture>(textureKey);

				if (texID != 0) {
					tilePaletteItems_.push_back({ name, PrefabType::Tile, texID, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f) });
					/*Framework::Logger::Get().Info(DebugType::Editor)
						<< " -> Success! Added tile: '" << name
						<< "' (ID: " << texID << ")" << std::endl;*/
				}
				else {
					Framework::Logger::Get().Error(DebugType::Editor)
						<< " -> Failed! Texture ID is 0 for tile: '" << name
						<< "' (Key: '" << textureKey << "')" << std::endl;
				}
			}
			catch (const std::out_of_range&) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< " -> Failed! Texture key '" << textureKey
					<< "' not found in ResourceManager for tile: '" << name << "'" << std::endl;
			}
			catch (const std::exception& e) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< " -> Failed! Exception getting texture for tile '" << name
					<< "' (Key: '" << textureKey << "'): " << e.what() << std::endl;
			}
			catch (...) {
				Framework::Logger::Get().Error(DebugType::Editor)
					<< " -> Failed! Unknown error getting texture for tile: '" << name
					<< "' (Key: '" << textureKey << "')" << std::endl;
			}
			};

		// Load tiles from platform prefabs
		for (const auto& pair : prefabSystem_.GetPlatformPrefabs()) {
			AddTileItem(pair.first, pair.second.textureID);
		}

		Framework::Logger::Get().Info(DebugType::Editor)
			<< "Prepared " << tilePaletteItems_.size() << " tile palette items." << std::endl;
	}

	// PREFAB PALETTE WINDOW
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

			if (isEraserActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

			if (ImGui::Selectable("Eraser", isEraserActive)) {
				if (isEraserActive) {
					// Toggle OFF
					isBrushMode_ = false;
					brushPayload_ = {};
					brushPayload_.type = PrefabType::None;
					if (dragDropPayload_.type == PrefabType::Eraser) {
						dragDropPayload_ = {};
						dragDropPayload_.type = PrefabType::None;
					}
					isDraggingPrefab_ = false;
				}
				else {
					// Toggle ON
					isBrushMode_ = true;
					brushPayload_ = { "Eraser", PrefabType::Eraser, 0 };
					isDraggingPrefab_ = false;
					dragDropPayload_ = {};
					dragDropPayload_.type = PrefabType::None;
				}
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

				bool clicked = false;

				if (item.textureID != 0) {
					// Normal image button for textured prefabs
					clicked = ImGui::ImageButton(item.name.c_str(),
						textureID,
						buttonSize,
						item.uv0,
						item.uv1,
						bgCol,
						tintCol);
				}
				else {
					// Fallback: colored button for non-textured prefabs (e.g. InvisibleCollider)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.6f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.7f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.5f, 1.0f));
					clicked = ImGui::Button(item.name.c_str(), buttonSize);
					ImGui::PopStyleColor(3);
				}

				if (clicked)
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
					if (item.textureID != 0) {
						ImGui::Image(textureID, ImVec2(32, 32), item.uv0, item.uv1);
					}
					isDraggingPrefab_ = true;
					dragDropPayload_ = item;
					ImGui::EndDragDropSource();
				}

				// Brush Mode (Right Click)
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

		ImGui::Begin("Hierarchy");

		SceneManager& sceneMgr = SceneManager::GetInstance();
		const SceneAsset& activeScene = sceneMgr.GetActiveScene();

		// --- Scene Info ---
		if (!activeScene.IsValid()) {
			ImGui::TextDisabled("No Scene Loaded!");
			ImGui::End();
			return;
		}

		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		// Add Search Bar
		static char searchBuffer[256] = "";
		ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
		ImGui::Spacing();

		if (ImGui::BeginChild("HierarchyList", ImVec2{ 0, 0 }, true)) {

			auto drawEntityItem = [&](Entity entity) {
				if (!entityMgr.ValidateEntity(entity)) return;

				const std::string& name = entityMgr.GetProperties(entity).name;

				ImGui::PushID(static_cast<int>(entity));

				// Check if this entity is in the selection
				bool selected = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) != selectedEntities_.end();
				
				// Keep backward compatibility with old single selection
				if (selectedEntityIndex > 0 && selectedEntities_.empty()) {
					selected = (static_cast<unsigned int>(selectedEntityIndex - 1) == entity);
				}

				if (!entityMgr.IsActive(entity)) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 160, 160, 255));
				}

				if (ImGui::Selectable(name.c_str(), &selected)) {
					// Flush any pending edits before changing selection
					editSession.Flush();
					selectedTagIndex = selectedComponentIndex = 0;
					
					ImGuiIO& io = ImGui::GetIO();
					
					if (io.KeyCtrl) {
						// Ctrl+Click: Toggle individual selection
						auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
						if (it != selectedEntities_.end()) {
							// Deselect if already selected
							selectedEntities_.erase(it);
						} else {
							// Add to selection
							selectedEntities_.push_back(entity);
						}
						lastClickedEntity_ = entity;
					}
					else if (io.KeyShift && lastClickedEntity_ != 0) {
						// Shift+Click: Range selection
						// Get all entities in the current scene
						SceneManagement::SceneGUID currentSceneID = activeScene.sceneIndex;
						if (entityMgr.HasComponent(entity, SceneOwnedType)) {
							currentSceneID = entityMgr.GetComponent<SceneOwned>(entity).sceneIndex;
						}
						
						const std::vector<Entity> allSceneEntities = sceneMgr.GetEntitiesInScene(currentSceneID);
						std::vector<Entity> sceneEntities;
						sceneEntities.reserve(allSceneEntities.size());
						for (const Entity e : allSceneEntities) {
							if (entityMgr.ValidateEntity(e)) {
								// Apply search filter
								const std::string& eName = entityMgr.GetProperties(e).name;
								std::string lowerName = eName;
								std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
								std::string lowerSearch = searchBuffer;
								std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
								
								if (lowerSearch.empty() || lowerName.find(lowerSearch) != std::string::npos) {
									sceneEntities.push_back(e);
								}
							}
						}
						
						// Sort entities alphabetically
						std::sort(sceneEntities.begin(), sceneEntities.end(),
							[&](Entity a, Entity b) {
								return entityMgr.GetProperties(a).name < entityMgr.GetProperties(b).name;
							});
						
						// Find indices of last clicked and current entity
						auto lastIt = std::find(sceneEntities.begin(), sceneEntities.end(), lastClickedEntity_);
						auto currentIt = std::find(sceneEntities.begin(), sceneEntities.end(), entity);
						
						if (lastIt != sceneEntities.end() && currentIt != sceneEntities.end()) {
							// Clear current selection
							selectedEntities_.clear();
							
							// Select range
							const size_t startIdx = static_cast<size_t>(std::min(std::distance(sceneEntities.begin(), lastIt),
								std::distance(sceneEntities.begin(), currentIt)));
							const size_t endIdx = static_cast<size_t>(std::max(std::distance(sceneEntities.begin(), lastIt),
								std::distance(sceneEntities.begin(), currentIt)));
							
							for (size_t i = startIdx; i <= endIdx; ++i) {
								selectedEntities_.push_back(sceneEntities[i]);
							}
						}
					}
					else {
						// Normal click: Single selection
						selectedEntities_.clear();
						selectedEntities_.push_back(entity);
						lastClickedEntity_ = entity;
						selectedEntityIndex = entity + 1;  // Keep for backward compatibility
					}
					
					isEntitySelected_ = !selectedEntities_.empty();
				}

				// Drag and Drop Source
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					// Pack the Entity ID into the payload
					ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(Entity));
					ImGui::Text("Entity: %s (ID: %u)", name.c_str(), entity);
					ImGui::EndDragDropSource();
				}

				if (!entityMgr.IsActive(entity)) {
					ImGui::PopStyleColor();
				}

				// Allows right-click selection
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					// Only change selection if entity is not already in the selection
					bool alreadySelected = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) != selectedEntities_.end();
					if (!alreadySelected) {
						// Flush any pending edits before changing selection
						editSession.Flush();
						selectedTagIndex = selectedComponentIndex = 0;
						selectedEntities_.clear();
						selectedEntities_.push_back(entity);
						selectedEntityIndex = entity + 1;
						lastClickedEntity_ = entity;
						isEntitySelected_ = true;
					}
				}

				// Selectable Context Menu
				if (ImGui::BeginPopupContextItem("##EntityContext")) {
					// Sanitize selection in case entities were deleted via undo/redo.
					selectedEntities_.erase(
						std::remove_if(selectedEntities_.begin(), selectedEntities_.end(),
							[&](Entity e) { return !entityMgr.ValidateEntity(e); }),
						selectedEntities_.end());
					isEntitySelected_ = !selectedEntities_.empty();
					if (isEntitySelected_) {
						selectedEntityIndex = selectedEntities_[0] + 1;
					}
					else {
						selectedEntityIndex = 0;
					}

					// Copy
					if (selectedEntities_.size() > 1) {
						if (ImGui::MenuItem(("Copy (" + std::to_string(selectedEntities_.size()) + " entities)").c_str())) {
							CopyEntitiesToClipboard(selectedEntities_);
						}
					} else {
						if (ImGui::MenuItem("Copy")) {
							CopyEntityToClipboard(entity);
						}
					}
					
					// Paste
					if (!entitiesClipboard_.empty()) {
						std::string pasteLabel = entitiesClipboard_.size() > 1 ? 
							"Paste (" + std::to_string(entitiesClipboard_.size()) + " entities)" : "Paste";
						if (ImGui::MenuItem(pasteLabel.c_str())) {
							PasteEntitiesFromClipboard();
						}
					}
					else {
						ImGui::TextDisabled("Paste");
					}

					ImGui::Separator();

					if (selectedEntities_.size() > 1) {
						// Duplicate multiple entities
						if (ImGui::MenuItem(("Duplicate (" + std::to_string(selectedEntities_.size()) + " entities)").c_str())) {
							std::vector<Entity> validSelection;
							for (Entity ent : selectedEntities_) {
								if (entityMgr.ValidateEntity(ent)) validSelection.push_back(ent);
							}
							if (validSelection.empty()) {
								ImGui::EndPopup();
								ImGui::PopID();
								return;
							}
							for (Entity ent : validSelection) {
								SceneManagement::SceneGUID ownerSceneID = activeScene.sceneIndex;
								if (entityMgr.HasComponent(ent, SceneOwnedType)) {
									ownerSceneID = entityMgr.GetComponent<SceneOwned>(ent).sceneIndex;
								}

								const SceneAsset& ownerScene = sceneMgr.GetSceneAt(ownerSceneID);
								Entity clonedEntity = entityMgr.CloneEntity(ent);
								sceneMgr.AddEntityToScene(ownerScene, clonedEntity);
							}
							historyManager_.Record(OpType::Create, validSelection[0], "Duplicated " + std::to_string(validSelection.size()) + " Entities");
						}
						
						// Delete multiple entities
						if (ImGui::MenuItem(("Delete (" + std::to_string(selectedEntities_.size()) + " entities)").c_str())) {
							for (Entity ent : selectedEntities_) {
								if (!entityMgr.ValidateEntity(ent)) continue;
								historyManager_.Record(OpType::Delete, ent, "Deleted Entity");
								entityMgr.DestroyEntity(ent);
							}
							selectedEntities_.clear();
							selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
							isEntitySelected_ = false;
						}
					} else {
						// Single entity operations
						if (ImGui::MenuItem("Duplicate")) {
							SceneManagement::SceneGUID ownerSceneID = activeScene.sceneIndex;
							if (entityMgr.HasComponent(entity, SceneOwnedType)) {
								ownerSceneID = entityMgr.GetComponent<SceneOwned>(entity).sceneIndex;
							}

							const SceneAsset& ownerScene = sceneMgr.GetSceneAt(ownerSceneID);
							Entity clonedEntity = entityMgr.CloneEntity(entity);
							sceneMgr.AddEntityToScene(ownerScene, clonedEntity);
							historyManager_.Record(OpType::Create, clonedEntity, "Duplicated Entity");
						}
						if (ImGui::MenuItem("Delete")) {
							historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
							entityMgr.DestroyEntity(entity);
							if (static_cast<unsigned int>((selectedEntityIndex - 1)) == entity) {
								selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
								isEntitySelected_ = false;
								selectedEntities_.clear();
							}
						}
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
				}; // End lambda: drawEntityItem

			auto drawSceneDropdown = [&](const SceneManagement::SceneGUID sceneID, const std::string& sceneName) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
				flags |= ImGuiTreeNodeFlags_DefaultOpen;

				ImGui::PushID(static_cast<int>(sceneID));

				bool open = ImGui::CollapsingHeader(sceneName.c_str(), flags);

				if (ImGui::BeginPopupContextItem("##SceneHeaderContext")) {
					// Paste
					if (!entitiesClipboard_.empty()) {
						std::string pasteLabel = entitiesClipboard_.size() > 1 ? 
							"Paste (" + std::to_string(entitiesClipboard_.size()) + " entities)" : "Paste";
						if (ImGui::MenuItem(pasteLabel.c_str())) {
							PasteEntitiesFromClipboard();
						}
					}
					else {
						ImGui::TextDisabled("Paste");
					}

					ImGui::Separator();

					// Create Empty Entity
					if (ImGui::MenuItem("Create Empty")) {
						Entity newEntity = entityMgr.CreateEntity();
						SceneManager::GetInstance().AddEntityToScene(sceneID, newEntity);
						historyManager_.Record(OpType::Create, newEntity, "Create Entity");
					}

					ImGui::EndPopup();
				} // End SceneHeaderContext

				if (open) {
					std::vector<Entity> list = sceneMgr.GetEntitiesInScene(sceneID);

					// Apply Search Filter
					std::string searchQuery = searchBuffer;
					if (!searchQuery.empty()) {
						// Convert query to lowercase for case-insensitive search
						std::transform(searchQuery.begin(), searchQuery.end(), searchQuery.begin(),
							[](unsigned char c) { return static_cast<char>(::tolower(c)); });

						list.erase(std::remove_if(list.begin(), list.end(), [&](Entity e) {
							std::string entityName = entityMgr.GetProperties(e).name;
							std::transform(entityName.begin(), entityName.end(), entityName.begin(),
								[](unsigned char c) { return static_cast<char>(::tolower(c)); });
							return entityName.find(searchQuery) == std::string::npos;
							}), list.end());
					}

					// Sort A-Z
					std::sort(list.begin(), list.end(), [&entityMgr](Entity a, Entity b) {
						return entityMgr.GetProperties(a).name < entityMgr.GetProperties(b).name;
						});

					if (list.empty()) {
						ImGui::TextDisabled(searchQuery.empty() ? "(Empty)" : "(No results)");
					}
					else {
						// Clipper for large list optimization(This only process entities in view in the list)
						ImGuiListClipper clipper;
						clipper.Begin(static_cast<int>(list.size()));
						while (clipper.Step()) {
							for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
								drawEntityItem(list[i]);
							}
						}
						clipper.End();
					}
				}
				ImGui::PopID();
				}; // End lambda: drawSceneDropdown

			// --- Per Scene Hierarchy ---
			const std::vector<SceneManagement::SceneGUID>& loadedScenes = sceneMgr.GetLoadedScenes();
			for (SceneGUID sceneID : loadedScenes) {
				if (sceneID == SceneManager::GetDDOLSceneID()) continue;
				const SceneAsset& scene = sceneMgr.GetSceneAt(sceneID, true);
				if (!scene.IsValid()) continue;

				std::string header = "Scene: " + scene.name;
				if (sceneID == activeScene.sceneIndex) header += " (Active)";
				drawSceneDropdown(sceneID, header);
			}

			if (IsGamePlaying()) {
				drawSceneDropdown(SceneManager::GetDDOLSceneID(), SceneManager::GetDDOLSceneName());
			}
			// --- End Per Scene Hierarchy ---
		}

		// Ensure Empty Space in Hierarchy List
		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.y > 0.f) {
			ImGui::InvisibleButton("##HierarchyEmptySpace", avail, ImGuiButtonFlags_MouseButtonRight);

			if (ImGui::BeginPopupContextItem("##HierarchyEmptyContext")) {
				// Paste
				if (!entitiesClipboard_.empty()) {
					std::string pasteLabel = entitiesClipboard_.size() > 1 ? 
						"Paste (" + std::to_string(entitiesClipboard_.size()) + " entities)" : "Paste";
					if (ImGui::MenuItem(pasteLabel.c_str())) {
						PasteEntitiesFromClipboard();
					}
				}
				else {
					ImGui::TextDisabled("Paste");
				}

				ImGui::Separator();

				// Create Empty Entity
				if (ImGui::MenuItem("Create Empty")) {
					Entity newEntity = entityMgr.CreateEntity();
					SceneManager::GetInstance().AddEntityToScene(activeScene, newEntity);
					historyManager_.Record(OpType::Create, newEntity, "Create Entity");
				}

				ImGui::EndPopup();
			} // End HierarchyEmptyContext
		}

		ImGui::EndChild();
		ImGui::End(); // Hierarchy End
	}

#pragma endregion

#pragma region INSPECTOR PANEL

	void Editor::ShowInspector() {
		using namespace Framework;

		ImGui::Begin("Inspector");
		if (isEntitySelected_) {
			EntityStorage& ecs = *(gameManager_->GetSystems().ecs_);
			EntityManager entityMgr(ecs);
			
			// Handle multiple entity selection
			if (selectedEntities_.size() > 1) {
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%zu entities selected", selectedEntities_.size());
				ImGui::Separator();
				
				// Show list of selected entities
				if (ImGui::CollapsingHeader("Selected Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
					for (size_t i = 0; i < selectedEntities_.size(); ++i) {
						Entity ent = selectedEntities_[i];
						if (entityMgr.ValidateEntity(ent)) {
							const std::string& name = entityMgr.GetProperties(ent).name;
							ImGui::BulletText("%s (ID: %u)", name.c_str(), ent);
						}
					}
				}
				
				ImGui::Spacing();
				ImGui::TextWrapped("Component editing is disabled when multiple entities are selected.");
				ImGui::TextWrapped("Select a single entity to edit its components.");
				
				ImGui::End();
				return;
			}
			
			// Single entity selection - use first entity or legacy selectedEntityIndex
			Entity selectedEntity;
			if (!selectedEntities_.empty()) {
				selectedEntity = selectedEntities_[0];
			} else {
				selectedEntity = static_cast<Entity>(selectedEntityIndex - 1);
			}

			// Check if Entity is alive before accessing properties.
			if (!entityMgr.ValidateEntity(selectedEntity)) {
				isEntitySelected_ = false;
				selectedEntityIndex = selectedComponentIndex = selectedTagIndex = selectedScriptIndex = 0;
				selectedEntities_.clear();
				ImGui::End();
				return;
			}

			// --- Name Field ---
			ImGui::TextDisabled("Entity ID: %u", selectedEntity);
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

			if (ImGui::InputText("##Entity Name", nameBuffer, sizeof(nameBuffer))) {

			}
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

			// --- Active Toggle Checkbox ---
			ImGui::SameLine();
			bool isActiveLocal = entityMgr.GetProperties(selectedEntity).isActive;
			if (ImGui::Checkbox("Is Active?", &isActiveLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);
				entityMgr.GetProperties(selectedEntity).isActive = isActiveLocal;
				std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified Is Active in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
				}
			}
			// --- End Active Toggle Checkbox ---

			// --- UUID ---
			std::string entityUUID = "UUID: ";
			entityUUID += std::to_string(entityMgr.GetProperties(selectedEntity).uuid);

			ImGui::TextDisabled(entityUUID.c_str());
			
			// --- End UUID ---

			// --- Layer Dropdown ---
			CollisionLayer& entityLayer = entityMgr.GetProperties(selectedEntity).layer;

			const auto& collisionLayers = layerManager_.GetCollisionLayers();

			// Create a list of layer names for the dropdown
			std::vector<std::string> layerLabels;
			std::vector<const char*> layerNames;

			for (const auto& layer : collisionLayers) {
				std::string label = std::to_string(layer.id) + ": " + layer.name;
				layerLabels.push_back(label);
			}

			for (const auto& label : layerLabels) {
				layerNames.push_back(label.c_str());
			}

			// Find current layer index
			int currentLayer = -1;
			for (size_t i = 0; i < collisionLayers.size(); ++i) {
				if (collisionLayers[i].id == static_cast<uint8_t>(entityLayer)) {
					currentLayer = static_cast<int>(i);

					// Update name if not set
					if (entityMgr.GetProperties(selectedEntity).collisionLayerName != collisionLayers[i].name) {
						entityMgr.GetProperties(selectedEntity).collisionLayerName = collisionLayers[i].name;
					}
					break;
				}
			}
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Layer");
			ImGui::SameLine(); // adjust offset spacing
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo("##Layer", &currentLayer, layerNames.data(), static_cast<int>(layerNames.size()))) {
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
			if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Attached Components:");

				int attachedCompCount = 0;
				for (const ComponentType type : componentTypes) {
					if (!entityMgr.HasComponent(selectedEntity, type)) {
						continue;
					}

					++attachedCompCount;

					// Preserve active edit tracking when drawing non-edited components.
					// This avoids a later component clearing editSession state for the one currently being edited.
					const bool preserveEditTracking = editSession.active && editSession.compInEdit != type;
					const bool previousWasItemActive = editSession.wasItemActive;
					if (preserveEditTracking) {
						editSession.wasItemActive = false;
					}

					ShowComponentSections(selectedEntity, type);

					if (preserveEditTracking) {
						editSession.wasItemActive = previousWasItemActive;
					}
				}

				if (attachedCompCount == 0) {
					ImGui::TextDisabled("No components attached.");
				}

				ImGui::SeparatorText("Manage Components");
				ImGui::Combo("Component", &selectedComponentIndex, componentItems.data(), static_cast<int>(componentItems.size()));

				if (selectedComponentIndex > 0) {
					const size_t compIndex = static_cast<size_t>(selectedComponentIndex - 1); // Offset for empty item
					if (compIndex < componentTypes.size()) {
						const ComponentType selectedComponent = componentTypes[compIndex];

						if (ImGui::Button("Add Component")) {
							if (!entityMgr.HasComponent(selectedEntity, selectedComponent)) {
								std::string beforeJSON;

								// Auto-calculate collider halfsize based on texture halfsize.
								if (selectedComponent == ComponentType::AABBColliderType) {
									AABBCollider newCollider;

									if (entityMgr.HasComponent(selectedEntity, ComponentType::TextureComponentType)) {
										const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(selectedEntity);
										newCollider.HalfSize = tex.HalfSize;
									}
									else {
										newCollider.HalfSize = { 50.0f, 50.0f };
									}

									entityMgr.AddComponent(selectedEntity, newCollider);
								}
								else {
									entityMgr.AddComponent(selectedEntity, selectedComponent);
								}

								const std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, ecs);
								const std::string description = "Add " + ComponentToString(selectedComponent);
								historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedComponent);
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove Component")) {
							if (entityMgr.HasComponent(selectedEntity, selectedComponent)) {
								const std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, ecs);
								entityMgr.RemoveComponent(selectedEntity, selectedComponent);

								std::string afterJSON;
								const std::string description = "Remove " + ComponentToString(selectedComponent);
								historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedComponent);
								selectedComponentIndex = 0;
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

				if (entityMgr.HasComponent(selectedEntity, ComponentType::LoadedTagType)) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
					ImGui::Text("[LoadedTag (Not Removable)]");
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

		const std::string componentLabel = ComponentToString(selectedComponent);

		ImGui::PushID(static_cast<int>(selectedComponent));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.10f, 0.35f, 0.75f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.45f, 0.90f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.08f, 0.30f, 0.65f, 1.0f));
		const bool isComponentOpen = ImGui::CollapsingHeader(componentLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopStyleColor(3);

		bool removeComponentFromHeader = false;
		if (ImGui::BeginPopupContextItem("##ComponentHeaderContext")) {
			if (ImGui::MenuItem("Delete Component")) {
				removeComponentFromHeader = true;
			}
			ImGui::EndPopup();
		}

		if (removeComponentFromHeader) {
			if (entityMgr.HasComponent(selectedEntity, selectedComponent)) {
				if (editSession.active && editSession.compInEdit == selectedComponent) {
					editSession.Flush();
				}

				const std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				entityMgr.RemoveComponent(selectedEntity, selectedComponent);

				std::string afterJSON;
				const std::string description = "Remove " + ComponentToString(selectedComponent);
				historyManager_.RecordModifyComponent(selectedEntity, description, beforeJSON, afterJSON, selectedComponent);
				selectedComponentIndex = 0;
			}
			ImGui::PopID();
			ImGui::Separator();
			return;
		}

		if (!isComponentOpen) {
			ImGui::PopID();
			return;
		}

		switch (selectedComponent) {
		case ComponentType::TransformType: {
			DrawTransformInspector(selectedEntity);
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
			DrawRigidBodyInspector(selectedEntity);
			break;
		}
		case ComponentType::AABBColliderType: {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(selectedEntity);

			ImGui::InputFloat2("HalfSize (X),(Y)", &col.HalfSize.x, "%.3f");
			bool activeNow = ImGui::IsItemActive();
			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}
			ImGui::InputFloat2("Offset (X),(Y)", &col.Offset.x, "%.3f");
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Offset the collider from the entity's center position.\nPositive X = right, Positive Y = up");
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
			if (col.Offset.x != 0.0f || col.Offset.y != 0.0f) {
				ImGui::SameLine();
				if (ImGui::Button("Reset Offset")) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					col.Offset = { 0.0f, 0.0f };
					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					historyManager_.RecordModifyComponent(selectedEntity, "Reset Collider Offset", beforeJSON, afterJSON, selectedComponent);
				}
			}

			editSession.wasItemActive = activeNow;
			break;
		}
		case ComponentType::ButtonType: {
			DrawButtonInspector(selectedEntity);
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
			//ImGui::InputFloat("Current Fall Distance", &trapComp.currentFallDistance, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
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
			DrawTextureInspector(selectedEntity);
			break;
		}
		case ComponentType::AudioComponentType: {
			DrawAudioInspector(selectedEntity);
			break;
		}

		case ComponentType::AnimationComponentType: {
			DrawAnimationInspector(selectedEntity);
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
			ImGui::InputText("Displayed Text", textBuffer, IM_ARRAYSIZE(textBuffer));
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
					std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); }); 

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

		case ComponentType::CameraType: {
			DrawCameraInspector(selectedEntity);
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
				// Derive the game's Source/ folder from the active game's Data/ dir.
				// Falls back to the legacy path when no game is imported.
				std::string sourceDir;
				const std::string& dataDir = FileIOSystem::GetSourceDataDir();
				if (!dataDir.empty()) {
					namespace fs = std::filesystem;
					sourceDir = (fs::path(dataDir).parent_path() / "Source" / "Native").string();
				}
				else {
					sourceDir = "../../owala/Native/";
				}
				NativeScriptMaker::CreateScriptPair(newScriptName, sourceDir, popupMsg);
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

		case ComponentType::FSMComponentType:
		{
			FSMComponent& fsm = entityMgr.GetComponent<FSMComponent>(selectedEntity);
			bool activeNow = false;

			// 1. Definition Path Input
			char buffer[256];
			strcpy_s(buffer, fsm.fsmDefinitionPath.c_str());

			if (ImGui::InputText("Definition Path", buffer, sizeof(buffer))) {
				fsm.fsmDefinitionPath = buffer;
				fsm.initialized = false; // Force rebuild
			}
			if (ImGui::IsItemActive()) activeNow = true;

			// --- DRAG AND DROP TARGET ---
			// Allows dragging a .json file from the Asset Browser into this field
			if (ImGui::BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload) {
					std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
					fs::path dropped_file(dropped_path);

					std::string ext = dropped_file.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

					if (ext == ".json") {
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						fs::path relative_path = fs::relative(dropped_path, fs::current_path());
						fsm.fsmDefinitionPath = relative_path.generic_string();
						fsm.initialized = false; // Force rebuild

						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified FSM Definition Path (DragDrop) in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
						Logger::Get().Info(DebugType::Editor) << "Assigned FSM Path: " << dropped_path << std::endl;
					}
				}
				ImGui::EndDragDropTarget();
			}
			// -----------------------------

			// 2. Initial State Override (Optional)
			char initBuffer[256];
			strcpy_s(initBuffer, sizeof(initBuffer), fsm.initialStateOverride.c_str());
			if (ImGui::InputText("Initial State Override", initBuffer, sizeof(initBuffer))) {
				fsm.initialStateOverride = initBuffer;
			}
			if (ImGui::IsItemActive()) activeNow = true;

			// 3. Current State Display (Read-Only)
			std::string currentState = fsm.fsm.GetCurrentState();
			ImGui::Text("Current State: %s", currentState.empty() ? "Uninitialized" : currentState.c_str());

			// 4. Reload Button
			if (ImGui::Button("Reload FSM")) {
				fsm.initialized = false;
			}

			if (activeNow && !editSession.wasItemActive) {
				editSession.Begin(selectedEntity, selectedComponent);
			}

			if (!activeNow && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Modified FSM Component in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
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

		case ComponentType::ParticleComponentType: {
			ParticleComponent& particle = entityMgr.GetComponent<ParticleComponent>(selectedEntity);

			std::string effectTypeName = "NONE";
			switch (particle.effectType) {
			case EffectType::NONE: effectTypeName = "NONE"; break;
			case EffectType::DRIFT: effectTypeName = "DRIFT"; break;
			case EffectType::RANDOM: effectTypeName = "RANDOM"; break;
			default: break;
			}

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

			char effectBuffer[128];
			strcpy_s(effectBuffer, sizeof(effectBuffer), effectTypeName.c_str());
			ImGui::InputText("Effect Type", effectBuffer, sizeof(effectBuffer), ImGuiInputTextFlags_ReadOnly);

			float lifetime = particle.lifetime;
			ImGui::InputFloat("Lifetime", &lifetime, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);

			float age = particle.age;
			ImGui::InputFloat("Age", &age, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);

			ImGui::PopStyleColor(2);

			bool isScreenSpace = particle.isScreenSpace;
			ImGui::BeginDisabled();
			ImGui::Checkbox("Screen Space?", &isScreenSpace);
			ImGui::EndDisabled();

			break;
		}

		case ComponentType::ParticleEmitterComponentType: {
			ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(selectedEntity);
			bool isEmittingLocal = emitter.isEmitting;
			bool isParticleAnimatedLocal = emitter.isParticleAnimated;
			bool isScreenSpaceLocal = emitter.isScreenSpace;
			bool activeNow = false;

			// Is Emitting Checkbox
			if (ImGui::Checkbox("Is Emitting?", &isEmittingLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.isEmitting = isEmittingLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
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
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Screen space toggle
			if (ImGui::Checkbox("Screen Space##screenspace", &isScreenSpaceLocal)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.isScreenSpace = isScreenSpaceLocal;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("If enabled, particles emit in screen/UI space (fixed to screen)\n"
					"If disabled, particles emit in world space (moves with camera)");
			}

			ImGui::Separator();
			ImGui::SeparatorText("Emission Mode");

			// Emission Mode Dropdown
			const char* emissionModeItems[] = { "CONTINUOUS", "BURST", "LOOP_BURST", "RANDOM_BURST"};
			int currentModeIndex = static_cast<int>(emitter.emissionMode);

			if (ImGui::Combo("Emission Mode##emitter", &currentModeIndex, emissionModeItems, IM_ARRAYSIZE(emissionModeItems))) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.emissionMode = static_cast<ParticleEmissionMode>(currentModeIndex);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Trigger Event Name (only show if not CONTINUOUS or RANDOM)
			if (emitter.emissionMode != ParticleEmissionMode::CONTINUOUS && emitter.emissionMode != ParticleEmissionMode::RANDOM_BURST) {
				ImGui::Text("Trigger Event Name:");
				ImGui::InputText("##TriggerEventName", &emitter.triggerEventName);
				if (ImGui::IsItemActive()) activeNow = true;

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Name of the event that triggers this emitter\n"
						"Examples: 'WorldRotation', 'PlayerDash', 'EnemyDeath'");
				}
			}

			// BURST-specific settings
			if (emitter.emissionMode == ParticleEmissionMode::BURST) {
				ImGui::Text("Burst Count (particles per burst):");
				ImGui::DragInt("##BurstCount", &emitter.burstCount, 1, 1, 100);
				if (ImGui::IsItemActive()) activeNow = true;
			}

			// LOOP_BURST-specific settings
			if (emitter.emissionMode == ParticleEmissionMode::LOOP_BURST) {
				ImGui::Text("Burst Count (particles per burst):");
				ImGui::DragInt("##BurstCount", &emitter.burstCount, 1, 1, 100);
				if (ImGui::IsItemActive()) activeNow = true;

				ImGui::Text("Burst Interval (seconds):");
				ImGui::DragFloat("##BurstInterval", &emitter.burstInterval, 0.01f, 0.01f, 5.0f, "%.2f");
				if (ImGui::IsItemActive()) activeNow = true;
			}

			// RANDOM_BURST-specific settings
			if (emitter.emissionMode == ParticleEmissionMode::RANDOM_BURST) {
				ImGui::Text("Burst Count Range (Min / Max):");
				ImGui::DragInt("##MinBurstCount", &emitter.minBurstCount, 1, 1, 100);
				if (ImGui::IsItemActive()) activeNow = true;
				ImGui::DragInt("##MaxBurstCount", &emitter.maxBurstCount, 1, 1, 100);
				if (ImGui::IsItemActive()) activeNow = true;

				// Ensure min does not exceed max
				if (emitter.minBurstCount > emitter.maxBurstCount) {
					emitter.minBurstCount = emitter.maxBurstCount;
				}

				ImGui::Text("Burst Interval Range (Min / Max seconds):");
				ImGui::DragFloat("##MinBurstInterval", &emitter.minBurstInterval, 0.01f, 0.01f, 10.0f, "%.2f");
				if (ImGui::IsItemActive()) activeNow = true;
				ImGui::DragFloat("##MaxBurstInterval", &emitter.maxBurstInterval, 0.01f, 0.01f, 10.0f, "%.2f");
				if (ImGui::IsItemActive()) activeNow = true;

				// Ensure min does not exceed max
				if (emitter.minBurstInterval > emitter.maxBurstInterval) {
					emitter.minBurstInterval = emitter.maxBurstInterval;
				}
			}

			// Texture Key
			ImGui::InputText("Texture Key##particle", &emitter.particleTextureKey, ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
				if (payload) {
					const char* dropped_path_c_str = (const char*)payload->Data;
					fs::path dropped_path(dropped_path_c_str);

					std::string ext = dropped_path.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); }); 

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
						std::string newKey = dropped_path.filename().string();
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						emitter.particleTextureKey = newKey;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
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
						ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
			}

			ImGui::Separator();
			ImGui::SeparatorText("Emission Properties");

			ImGui::InputFloat("Particle Lifetime", &emitter.particleLifetime);
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat("Emission Per Second", &emitter.emissionRate);
			if (ImGui::IsItemActive()) activeNow = true;
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
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
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
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
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
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.emissionStyle = static_cast<EmissionStyle>(currentStyleIndex);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}

			// Emission Offset
			ImGui::Spacing();
			ImGui::Text("Emission Offset:");
			ImGui::SetNextItemWidth(200.0f);
			ImGui::DragFloat2("Offset (X, Y)", &emitter.emissionOffset.x, 0.01f, -10.0f, 10.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;

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
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
				emitter.emissionOffset = { 0.0f, 0.0f };
				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified ParticleEmitter in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
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
				if (ImGui::IsItemActive()) activeNow = true;
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
					if (ImGui::IsItemActive()) activeNow = true;
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
				}
				ImGui::TextWrapped("Particles spawn at the top and fall down");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_LEFT_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
				}
				ImGui::TextWrapped("Particles spawn at the left edge and move right");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_RIGHT_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
				}
				ImGui::TextWrapped("Particles spawn at the right edge and move left");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_BOTTOM_VIEWPORT:
				if (!emitter.isScreenSpace) {
					ImGui::InputFloat2("Viewport Min (bottom-left)", &emitter.viewportMin.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
					ImGui::InputFloat2("Viewport Max (top-right)", &emitter.viewportMax.x, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
				}
				ImGui::TextWrapped("Particles spawn at the bottom and move up");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_LINE:
				ImGui::InputFloat2("Line Start", &emitter.lineStart.x, "%.2f");
				if (ImGui::IsItemActive()) activeNow = true;
				ImGui::InputFloat2("Line End", &emitter.lineEnd.x, "%.2f");
				if (ImGui::IsItemActive()) activeNow = true;
				ImGui::TextWrapped("Particles spawn randomly along a line");
				if (emitter.isScreenSpace) {
					ImGui::TextDisabled("(Using screen space: 0-1 normalized coordinates)");
				}
				break;

			case EmissionStyle::FROM_CIRCLE_EDGE:
				ImGui::InputFloat("Circle Radius##circleradius", &emitter.circleRadius, 0.05f, 0.1f);
				if (ImGui::IsItemActive()) activeNow = true;
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
				if (ImGui::IsItemActive()) activeNow = true;
				ImGui::TextWrapped("Direction vector for particle movement (normalize for consistent speed)");
			}

			ImGui::Separator();
			ImGui::SeparatorText("Advanced");
			ImGui::InputFloat2("Particle Start Velocity", &emitter.particleStartVelocity.x, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			ImGui::InputFloat2("Particle Acceleration", &emitter.particleAcceleration.x, "%.2f");
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

		case ComponentType::SliderType: {
			Slider& slider = entityMgr.GetComponent<Slider>(selectedEntity);
			bool activeNow = false;

			// Slider Sound Type Dropdown
			const char* sliderTypeItems[] = { "Master Volume", "Music Volume", "SFX Volume" };
			int currentSliderTypeIndex = static_cast<int>(slider.sliderType);

			if (ImGui::Combo("Control Type", &currentSliderTypeIndex, sliderTypeItems, IM_ARRAYSIZE(sliderTypeItems))) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				// Update the slider type
				slider.sliderType = static_cast<SliderSoundType>(currentSliderTypeIndex);

				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				// Record for Undo/Redo
				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified Slider Type in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}
			ImGui::Separator();

			// Helper to render a drag-drop entity reference slot.
			// Displays the assigned entity name and accepts HIERARCHY_ENTITY drag-drop to assign by UUID.
			auto EntityRefSlot = [&](const char* label, uint64_t& uuid, Entity& cachedEntity) {
				// Resolve display name from cached entity or UUID lookup
				std::string displayName = "(None)";
				if (uuid != 0) {
					if (!entityMgr.ValidateEntity(cachedEntity)) {
						cachedEntity = entityMgr.GetEntityByUUID(uuid);
					}
					displayName = entityMgr.ValidateEntity(cachedEntity)
						? entityMgr.GetProperties(cachedEntity).name
						: "(Missing)";
				}

				// Button acts as the drag-drop target
				ImGui::Button((displayName + "##" + label).c_str(), ImVec2(-60.f, 0.f));
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
						Entity droppedEntity = *static_cast<const Entity*>(payload->Data);
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

						uuid        = entityMgr.GetProperties(droppedEntity).uuid;
						cachedEntity = droppedEntity;
						slider.needsResolution = true;

						std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
						if (beforeJSON != afterJSON) {
							std::stringstream ss;
							ss << "Assigned " << label << " in " << entityMgr.GetProperties(selectedEntity).name;
							historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
						}
					}
					ImGui::EndDragDropTarget();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drag an entity from the Hierarchy");

				ImGui::SameLine();
				if (ImGui::Button(("Clear##" + std::string(label)).c_str())) {
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

					uuid         = 0;
					cachedEntity = 0;
					slider.needsResolution = true;

					std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);
					if (beforeJSON != afterJSON) {
						std::stringstream ss;
						ss << "Cleared " << label << " in " << entityMgr.GetProperties(selectedEntity).name;
						historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
					}
				}
				ImGui::SameLine();
				ImGui::TextDisabled("UUID: %llu", uuid);
			};

			ImGui::Text("Handle Entity"); ImGui::SameLine();
			EntityRefSlot("Handle", slider.handleUUID, slider.handleEntity);
			ImGui::Text("Fill Entity  "); ImGui::SameLine();
			EntityRefSlot("Fill", slider.fillUUID, slider.fillEntity);

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				slider.fillEntity = 0;
				slider.needsResolution = true;
			}

			// Current Value
			ImGui::InputFloat("Current Value", &slider.currentValue, 0.1f, 1.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			// Min Value
			ImGui::InputFloat("Min Value", &slider.minValue, 0.1f, 1.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			// Max Value
			ImGui::InputFloat("Max Value", &slider.maxValue, 0.1f, 1.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			// Width
			ImGui::InputFloat("Width", &slider.width, 1.0f, 10.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			// X Offset
			ImGui::InputFloat("Track Offset X", &slider.widthOffsetX, 1.0f, 10.0f, "%.2f");
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shifts the slidable area left or right from the center");

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
		
		case ComponentType::PlayerHealthComponentType: {
			PlayerHealthComponent& healthComp = entityMgr.GetComponent<PlayerHealthComponent>(selectedEntity);

			int newLifeNum = healthComp.lifeNum;
			if (ImGui::InputInt("Life number:", &newLifeNum)) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				healthComp.lifeNum = std::max(0, newLifeNum);

				std::string afterJSON = EntitySerializer::SerializeComponentToString(selectedEntity, selectedComponent, *ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified PlayerHealth in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON, selectedComponent);
				}
			}
			break;
		}
		default:
			break;
		}
		ImGui::Separator();
		ImGui::PopID();
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

		// Handle inputs for gizmos/modes (QTYUI)
		if (gizmoSystem_) {
			// I: Marquee mode
			if (ImGui::IsKeyPressed(ImGuiKey_I)) {
				isMarqueeMode_ = true;
				isMarqueeSelecting_ = false;
				gizmoSystem_->SetGizmoMode(GizmoMode::None);
			}
			// Q: Select/View Mode (No Gizmo, no marquee)
			else if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
				isMarqueeMode_ = false;
				isMarqueeSelecting_ = false;
				gizmoSystem_->SetGizmoMode(GizmoMode::None);
			}
			// T: Translate
			else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
				isMarqueeMode_ = false;
				isMarqueeSelecting_ = false;
				gizmoSystem_->SetGizmoMode(GizmoMode::Translate);
			}
			// Y: Rotate
			else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
				isMarqueeMode_ = false;
				isMarqueeSelecting_ = false;
				gizmoSystem_->SetGizmoMode(GizmoMode::Rotate);
			}
			// U: Scale
			else if (ImGui::IsKeyPressed(ImGuiKey_U)) {
				isMarqueeMode_ = false;
				isMarqueeSelecting_ = false;
				gizmoSystem_->SetGizmoMode(GizmoMode::Scale);
			}
		}

		// Camera Controls (WASD, Up/Down Arrows, R)
		if (editorCamera_ != 0) {
			Framework::Camera& cam = cameraSystem_->GetCamera(editorCamera_);

			// Mark this camera as editor-controlled
			cam.isEditorControlled = true;

			const float dt = Framework::Time::UnscaledDeltaTime();
			const float camSpeed = 2000.0f;

			if (ImGui::IsKeyDown(ImGuiKey_W)) cam.position.y += camSpeed * dt;
			if (ImGui::IsKeyDown(ImGuiKey_S)) cam.position.y -= camSpeed * dt;
			if (ImGui::IsKeyDown(ImGuiKey_A)) cam.position.x -= camSpeed * dt;
			if (ImGui::IsKeyDown(ImGuiKey_D)) cam.position.x += camSpeed * dt;
			// Zoom in/out using Up / Down arrow keys
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
				cam.zoom *= 0.8f; // zoom in
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
				cam.zoom *= 1.2f; // zoom out
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}

			float scrollDelta = ImGui::GetIO().MouseWheel;
			if (scrollDelta != 0.0f && isMouseOverGameViewport_) {
				if (scrollDelta > 0.0f) {
					cam.zoom *= 0.8f;
				}
				else if (scrollDelta < 0.0f) {
					cam.zoom *= 1.2f;
				}
				cameraSystem_->SetZoom(editorCamera_, cam.zoom);
			}

			// Optional: reset zoom with R
			if (ImGui::IsKeyPressed(ImGuiKey_R)) {
				ResetEditorCamera();
			}
		}

		// Delete selected entity/entities with Delete key
		if (isEntitySelected_) {
			if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
				
				// Delete all selected entities
				if (!selectedEntities_.empty()) {
					for (Entity entity : selectedEntities_) {
						if (entityMgr.ValidateEntity(entity)) {
							historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
							entityMgr.DestroyEntity(entity);
						}
					}
					selectedEntities_.clear();
					selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
					isEntitySelected_ = false;
				}
				// Legacy single entity deletion
				else if (selectedEntityIndex > 1) {
					Entity entity = selectedEntityIndex - 1;
					historyManager_.Record(OpType::Delete, entity, "Deleted Entity");
					entityMgr.DestroyEntity(entity);
					if (static_cast<unsigned int>((selectedEntityIndex - 1)) == entity) {
						selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
						isEntitySelected_ = false;
					}
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

				// Calculate the aspect ratio of the specific ImGui Window
				float viewportAspect = viewportPanelSize.x / viewportPanelSize.y;

				// Determine which camera is currently being used
				Entity cameraInUse = 0;
				if (engine_->IsEditorMode() && editorCamera_ != 0) {
					cameraInUse = editorCamera_;
				}
				else {
					// If in Play Mode, we are using the Game's active camera
					cameraInUse = camSystem->GetActiveCamera();
				}

				// Force that camera to match the ImGui window dimensions
				if (cameraInUse != 0) {
					camSystem->SetAspectRatio(cameraInUse, viewportAspect);
				}

				// === PREPARE FRAMEBUFFER ===
				gfx->ResizeGameFramebuffer(static_cast<unsigned int>(viewportPanelSize.x), static_cast<unsigned int>(viewportPanelSize.y));
				gfx->BindGameFramebuffer();
				gfx->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f }); // clear frame buffer first

				// ===  TEMPORARILY SWITCH TO EDITOR CAMERA ===
				Entity prevCamera = camSystem->GetActiveCamera();

				// Save current editor camera state before any rendering
				if (editorCamera_ != 0 && engine_->IsEditorMode()) {
					SaveEditorCameraState();
				}

				if (engine_->IsEditorMode() && editorCamera_ != 0) {
					//  Update Aspect Ratio based on the actual ImGui Window size
					float aspect = (viewportPanelSize.y > 0) ? (viewportPanelSize.x / viewportPanelSize.y) : 1.0f;
					camSystem->SetAspectRatio(editorCamera_, aspect);

					//  Manually update the camera.size. 
					Framework::Camera& cam = camSystem->GetCamera(editorCamera_);
					cam.size.x = 2.0f * cam.zoom * cam.aspectRatio;
					cam.size.y = 2.0f * cam.zoom;

					camSystem->SetActiveCamera(editorCamera_);
				}

				// ===  RENDER TEXTS & SPRITES ===
				textRenderer_->Update();
				spriteRenderer_->Update();

				// ===  RESTORE ORIGINAL CAMERA ===
				if (engine_->IsEditorMode()) {
					camSystem->SetActiveCamera(prevCamera);

					// Ensure editor camera state is preserved
					if (prevCamera == editorCamera_ && editorCamera_ != 0) {
						RestoreEditorCameraState();
					}
				}

				// ===  UNBIND FRAMEBUFFER ===
				gfx->UnbindGameFramebuffer();

				// === DRAW GAME TEXTURE ===
				ImGui::Image((void*)(intptr_t)gfx->GetGameTexture(),
					viewportPanelSize,
					ImVec2(0, 1), ImVec2(1, 0));

				// ===  DRAW OVERLAYS (GRID, DEBUG) ===
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImVec2 clipMin = gameViewportPos_;
				ImVec2 clipMax = ImVec2(gameViewportPos_.x + gameViewportSize_.x,
					gameViewportPos_.y + gameViewportSize_.y);
				drawList->PushClipRect(clipMin, clipMax, true);

				if (showGrid_) {
					DrawEditorGrid(drawList);
				}

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

#pragma region FSM
	void Editor::LoadFSMDefinitionsFromFolder(const std::string& folderPath) {
		using namespace Framework;

		availableFSMDefinitions_.clear();

		if (!std::filesystem::exists(folderPath)) {
			Logger::Get().Error(DebugType::Editor)
				<< "FSM definitions folder not found: " << folderPath << std::endl;
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				availableFSMDefinitions_.push_back(entry.path().string());
				Logger::Get().Info(DebugType::Editor)
					<< "Found FSM definition: " << entry.path().filename().string() << std::endl;
			}
		}
	}

	void Editor::PrepareFSMPaletteItems() {
		using namespace Framework;

		fsmPaletteItems_.clear();

		// Load FSM definitions from folder
		LoadFSMDefinitionsFromFolder("Data/FSM/");

		for (const std::string& fsmPath : availableFSMDefinitions_) {
			// Parse the JSON to get display info
			rapidjson::Document doc;
			if (!FileIOSystem::ReadFile(fsmPath, doc)) {
				Logger::Get().Error(DebugType::Editor)
					<< "Failed to read FSM definition: " << fsmPath << std::endl;
				continue;
			}

			std::string displayName = std::filesystem::path(fsmPath).stem().string();
			std::string textureKey = "default_enemy.png"; // Default texture
			ImVec2 halfSize{ 25.0f, 25.0f };

			// Try to extract texture from the definition
			if (doc.HasMember("previewTexture") && doc["previewTexture"].IsString()) {
				textureKey = doc["previewTexture"].GetString();
			}

			if (doc.HasMember("halfSize") && doc["halfSize"].IsObject()) {
				const auto& sizeObj = doc["halfSize"];
				if (sizeObj.HasMember("x")) halfSize.x = sizeObj["x"].GetFloat();
				if (sizeObj.HasMember("y")) halfSize.y = sizeObj["y"].GetFloat();
			}

			// Get texture ID
			try {
				Texture texID = resourceManager_->get<Texture>(textureKey);

				if (texID != 0) {
					fsmPaletteItems_.push_back({
						displayName,
						fsmPath,
						textureKey,
						(ImTextureID)(intptr_t)texID,
						ImVec2(0.0f, 1.0f),
						ImVec2(1.0f, 0.0f),
						halfSize
						});

					Logger::Get().Info(DebugType::Editor)
						<< "Added FSM palette item: " << displayName << std::endl;
				}
			}
			catch (const std::exception& e) {
				Logger::Get().Error(DebugType::Editor)
					<< "Failed to load texture for FSM: " << displayName
					<< " - " << e.what() << std::endl;
			}
		}

		Logger::Get().Info(DebugType::Editor)
			<< "Prepared " << fsmPaletteItems_.size() << " FSM palette items." << std::endl;
	}

	void Editor::ShowFSMEditor() {
		using namespace Framework;

		static bool wasOpen = false;

		if (ImGui::Begin("FSM Enemy Editor", &isFSMEditorOpen_)) {
			if (!wasOpen) {
				wasOpen = true;
				// Prepare palette when opening
				if (!fsmPalettePrepared_) {
					PrepareFSMPaletteItems();
					fsmPalettePrepared_ = true;
				}
			}

			ImGui::Text("Drag and drop FSM enemies into the scene");
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				"Left Click to drag, or select and click in scene");
			ImGui::Separator();

			// Reload button
			if (ImGui::Button("Reload FSM Definitions")) {
				fsmPalettePrepared_ = false;
				PrepareFSMPaletteItems();
				fsmPalettePrepared_ = true;
			}

			ImGui::Separator();

			// Display FSM palette items
			ImVec2 buttonSize(96, 96);
			ImVec4 bgCol(0, 0, 0, 0);
			ImVec4 tintCol(1, 1, 1, 1);

			int itemsPerRow = 0;
			const int maxItemsPerRow = 4;

			if (fsmPaletteItems_.empty()) {
				ImGui::TextDisabled("No FSM definitions found in Data/FSM/");
				ImGui::TextDisabled("Create .json files with FSM definitions");
			}

			for (const auto& item : fsmPaletteItems_) {
					ImGui::PushID(item.name.c_str());
					ImGui::BeginGroup();

					// Button with preview image
					if (ImGui::ImageButton(
						item.name.c_str(),
						item.textureID,
						buttonSize,
						item.uv0,
						item.uv1,
						bgCol,
						tintCol))
					{
						selectedFSMDefinition_ = item.fsmPath;
						Logger::Get().Info(DebugType::Editor)
							<< "Selected FSM: " << item.name << std::endl;
					}

					// Drag and drop source
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
						fsmDragDropPayload_ = item;
						ImGui::SetDragDropPayload("FSM_ENEMY_DND",
							fsmDragDropPayload_.fsmPath.c_str(),
							fsmDragDropPayload_.fsmPath.size() + 1);
						ImGui::Text("Placing: %s", item.name.c_str());
						ImGui::Image(item.textureID, ImVec2(48, 48), item.uv0, item.uv1);

						isDraggingFSM_ = true;
						ImGui::EndDragDropSource();
					}

					// Show tooltip
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("%s", item.name.c_str());
						ImGui::TextDisabled("Path: %s", item.fsmPath.c_str());
						ImGui::EndTooltip();
					}

					// Display name below button
					ImGui::Text("%s", item.name.c_str());

					ImGui::EndGroup();
					ImGui::PopID();

					itemsPerRow++;
					if (itemsPerRow < maxItemsPerRow) {
						ImGui::SameLine();
					}
					else {
						itemsPerRow = 0;
					}
				}

			if (itemsPerRow != 0) {
				ImGui::NewLine();
			}

			ImGui::Separator();

			// Instructions
			if (ImGui::CollapsingHeader("Instructions", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::BulletText("Click an FSM enemy to select it");
				ImGui::BulletText("Drag and drop into the game viewport to place");
				ImGui::BulletText("Or click in viewport after selection");
				ImGui::BulletText("FSM definitions are loaded from Data/FSM/");
			}

		}
		ImGui::End();

		if (!isFSMEditorOpen_ && wasOpen) {
			wasOpen = false;
			isDraggingFSM_ = false;
			selectedFSMDefinition_.clear();
		}
	}


	Entity Editor::CreateFSMEntityAtPosition(const std::string& fsmPath, const Vector2D& position) {
		using namespace Framework;

		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);

		// Create entity
		Entity enemy = entityMgr.CreateEntity();

		// Parse FSM definition to get initial setup
		rapidjson::Document doc;
		if (!FileIOSystem::ReadFile(fsmPath, doc)) {
			Logger::Get().Error(DebugType::Editor)
				<< "Failed to load FSM definition: " << fsmPath << std::endl;
			entityMgr.DestroyEntity(enemy);
			return 0;
		}

		// Set properties
		std::string enemyName = std::filesystem::path(fsmPath).stem().string();
		entityMgr.GetProperties(enemy).name = enemyName;
		entityMgr.GetProperties(enemy).layer = CollisionLayer::Enemy;

		// Add required components for FSM
		entityMgr.AddComponent<Transform>(enemy, Transform(position, 0.f));
		entityMgr.AddComponent<Velocity>(enemy, Velocity{});
		entityMgr.AddComponent<EnemyTag>(enemy, EnemyTag{});

		// Add FSM Component
		FSMComponent fsmComp;
		fsmComp.fsmDefinitionPath = fsmPath;
		fsmComp.initialized = false; // Will be initialized by AISystem

		// Check for initial state override in JSON
		if (doc.HasMember("initialState") && doc["initialState"].IsString()) {
			fsmComp.initialStateOverride = doc["initialState"].GetString();
		}

		entityMgr.AddComponent(enemy, fsmComp);

		// Add visual components 
		std::string textureKey = "default_enemy.png";
		Vector2D halfSize = { 50.f, 50.f };

		if (doc.HasMember("texture") && doc["texture"].IsString()) {
			textureKey = doc["texture"].GetString();
		}
		if (doc.HasMember("halfSize") && doc["halfSize"].IsObject()) {
			const auto& sizeObj = doc["halfSize"];
			if (sizeObj.HasMember("x")) halfSize.x = sizeObj["x"].GetFloat();
			if (sizeObj.HasMember("y")) halfSize.y = sizeObj["y"].GetFloat();
		}

		entityMgr.AddComponent<TextureComponent>(enemy, TextureComponent{ textureKey, halfSize });

		// Only add collision if not explicitly disabled (e.g., witch phases through things)
		bool hasCollision = true;
		if (doc.HasMember("hasCollision") && doc["hasCollision"].IsBool()) {
			hasCollision = doc["hasCollision"].GetBool();
		}
		if (hasCollision) {
			entityMgr.AddComponent<AABBCollider>(enemy, AABBCollider{ halfSize });
		}

		// Add physics
		RigidBody rb;
		rb.SetMass(1.0f);
		rb.isAffectedByGravity = false; // Most AI enemies float/fly
		entityMgr.AddComponent(enemy, rb);

		// Add animation 
		if (doc.HasMember("animationProfile") && doc["animationProfile"].IsString()) {
			std::string animProfile = doc["animationProfile"].GetString();
			entityMgr.AddComponent<AnimationComponent>(enemy, AnimationComponent{ animProfile });
		}

		// Add rotation flag 
		entityMgr.AddComponent<RotationFlag>(enemy, RotationFlag{});

		Logger::Get().Info(DebugType::Editor)
			<< "Created FSM enemy: " << enemyName
			<< " at (" << position.x << ", " << position.y << ")" << std::endl;

		return enemy;
	}

	void Editor::HandleFSMDragDropPlacement() {
		if (!isDraggingFSM_ || !isMouseOverGameViewport_) {
			isDraggingFSM_ = false;
			return;
		}

		Vector2D worldMousePos = GetMouseWorldPosition();
		Vector2D snappedPos = SnapToGrid(worldMousePos);

		// Create the FSM enemy
		Entity newEnemy = CreateFSMEntityAtPosition(fsmDragDropPayload_.fsmPath, snappedPos);

		if (newEnemy != 0) {
			Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
			sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), newEnemy);

			historyManager_.Record(OpType::Create, newEnemy, "Created FSM Enemy");
		}

		isDraggingFSM_ = false;
	}

#pragma endregion

#pragma region LAYER
	void Editor::ShowLayerManager() {

		
	ImGui::Begin("Layer");

	// Add button at the top to open layer configuration
	if (ImGui::Button("Layer Settings", ImVec2(120, 0))) {
		showLayerConfigWindow_ = true;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Configure render and collision layers");

	ImGui::Separator();

	auto* physics = gameManager_->GetSystems().physics_;
	if (!physics) {
		ImGui::TextDisabled("Physics system not available");
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Scene Entity Layer Tracker", ImGuiTreeNodeFlags_DefaultOpen)) {
		Framework::EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
		ShowSceneEntityTracker(entityMgr);
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

	ShowLayerConfigWindow();
	
	}

	void Editor::ShowLayerConfigWindow() {
		if (!showLayerConfigWindow_) return;

		ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Layer Configuration", &showLayerConfigWindow_)) {
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("LayerTabs")) {
			// === RENDER LAYERS TAB ===
			if (ImGui::BeginTabItem("Render Layers")) {
				ShowRenderLayersTab();
				ImGui::EndTabItem();
			}

			// === COLLISION LAYERS TAB ===
			if (ImGui::BeginTabItem("Collision Layers")) {
				ShowCollisionLayersTab();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void Editor::ShowRenderLayersTab() {
		ImGui::TextWrapped("Render layers control the draw order of sprites. Lower values render first (behind).");
		ImGui::Separator();

		// === ADD NEW LAYER ===
		if (ImGui::CollapsingHeader("Add New Render Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Layer Name", newRenderLayerName_, IM_ARRAYSIZE(newRenderLayerName_));
			ImGui::DragInt("Render Order", &newRenderLayerOrder_, 1.0f, -1000, 1000);

			if (ImGui::Button("Add Layer", ImVec2(120, 0))) {
				int idx = layerManager_.AddRenderLayer(newRenderLayerName_, newRenderLayerOrder_);
				if (idx >= 0) {
					Logger::Get().Info(DebugType::Editor)
						<< "Added render layer: " << newRenderLayerName_ << std::endl;
					// Reset fields
					strcpy(newRenderLayerName_, "New Layer");
					newRenderLayerOrder_ = 0;

					// Save immediately
					layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH);
				}
			}
		}

		ImGui::Separator();

		// === EXISTING LAYERS LIST ===
		ImGui::Text("Existing Render Layers:");

		if (ImGui::BeginChild("RenderLayerList", ImVec2(0, 250), true)) {
			const auto& layers = layerManager_.GetRenderLayers();

			for (size_t i = 0; i < layers.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));

				bool isSelected = (selectedRenderLayerIdx_ == static_cast<int>(i));

				// Color built-in layers differently
				if (layers[i].isBuiltIn) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
				}

				std::string label = layers[i].name + " (" + std::to_string(layers[i].renderOrder) + ")";
				if (layers[i].isBuiltIn) {
					label += " [Built-in]";
				}

				if (ImGui::Selectable(label.c_str(), isSelected)) {
					selectedRenderLayerIdx_ = static_cast<int>(i);
				}

				if (layers[i].isBuiltIn) {
					ImGui::PopStyleColor();
				}

				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		// === EDIT SELECTED LAYER ===
		if (selectedRenderLayerIdx_ >= 0 &&
			selectedRenderLayerIdx_ < static_cast<int>(layerManager_.GetRenderLayers().size())) {

			const auto& layer = layerManager_.GetRenderLayers()[selectedRenderLayerIdx_];

			ImGui::Separator();
			ImGui::Text("Edit Selected Layer:");

			static char editName[128] = "";
			static int editOrder = 0;
			static bool firstEdit = true;

			if (firstEdit) {
				strcpy(editName, layer.name.c_str());
				editOrder = layer.renderOrder;
				firstEdit = false;
			}

			bool canEdit = !layer.isBuiltIn;

			if (!canEdit) {
				ImGui::BeginDisabled();
			}

			ImGui::InputText("Name##Edit", editName, IM_ARRAYSIZE(editName));
			ImGui::DragInt("Order##Edit", &editOrder, 1.0f, -1000, 1000);

			if (ImGui::Button("Update", ImVec2(100, 0))) {
				if (layerManager_.UpdateRenderLayer(selectedRenderLayerIdx_, editName, editOrder)) {
					Logger::Get().Info(DebugType::Editor)
						<< "Updated render layer: " << editName << std::endl;
					firstEdit = true;
					layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH);
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Delete", ImVec2(100, 0))) {
				if (layerManager_.RemoveRenderLayer(selectedRenderLayerIdx_)) {
					Logger::Get().Info(DebugType::Editor)
						<< "Deleted render layer" << std::endl;
					selectedRenderLayerIdx_ = -1;
					firstEdit = true;
					layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH);
				}
			}

			if (!canEdit) {
				ImGui::EndDisabled();
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
					"Built-in layers cannot be edited or deleted");
			}
		}
	}

	void Editor::ShowCollisionLayersTab() {
		ImGui::TextWrapped("Collision layers control which objects can collide with each other.");
		ImGui::Separator();

		// === ADD NEW LAYER ===
		if (ImGui::CollapsingHeader("Add New Collision Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Layer Name", newCollisionLayerName_, IM_ARRAYSIZE(newCollisionLayerName_));

			if (ImGui::Button("Add Layer", ImVec2(120, 0))) {
				int id = layerManager_.AddCollisionLayer(newCollisionLayerName_);
				if (id >= 0) {
					Logger::Get().Info(DebugType::Editor)
						<< "Added collision layer: " << newCollisionLayerName_
						<< " (ID: " << id << ")" << std::endl;
					strcpy(newCollisionLayerName_, "New Layer");
					layerManager_.SaveToFile();
				}
			}

			ImGui::SameLine();
			ImGui::TextDisabled("(%d/32 layers used)",
				static_cast<int>(layerManager_.GetCollisionLayers().size()));
		}

		ImGui::Separator();

		// === EXISTING LAYERS LIST ===
		ImGui::Text("Existing Collision Layers:");

		if (ImGui::BeginChild("CollisionLayerList", ImVec2(0, 250), true)) {
			const auto& layers = layerManager_.GetCollisionLayers();

			for (size_t i = 0; i < layers.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));

				bool isSelected = (selectedCollisionLayerIdx_ == static_cast<int>(i));

				if (layers[i].isBuiltIn) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
				}

				std::string label = layers[i].name + " (ID: " + std::to_string(layers[i].id) + ")";
				if (layers[i].isBuiltIn) {
					label += " [Built-in]";
				}

				if (ImGui::Selectable(label.c_str(), isSelected)) {
					selectedCollisionLayerIdx_ = static_cast<int>(i);
				}

				if (layers[i].isBuiltIn) {
					ImGui::PopStyleColor();
				}

				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		// === EDIT SELECTED LAYER ===
		if (selectedCollisionLayerIdx_ >= 0 &&
			selectedCollisionLayerIdx_ < static_cast<int>(layerManager_.GetCollisionLayers().size())) {

			const auto& layer = layerManager_.GetCollisionLayers()[selectedCollisionLayerIdx_];

			ImGui::Separator();
			ImGui::Text("Edit Selected Layer:");

			static char editColName[128] = "";
			static bool firstColEdit = true;

			if (firstColEdit) {
				strcpy(editColName, layer.name.c_str());
				firstColEdit = false;
			}

			bool canEdit = !layer.isBuiltIn;

			if (!canEdit) {
				ImGui::BeginDisabled();
			}

			ImGui::InputText("Name##ColEdit", editColName, IM_ARRAYSIZE(editColName));
			ImGui::Text("ID: %d (Read-only)", layer.id);

			if (ImGui::Button("Rename", ImVec2(100, 0))) {
				if (layerManager_.RenameCollisionLayer(layer.id, editColName)) {
					Logger::Get().Info(DebugType::Editor)
						<< "Renamed collision layer to: " << editColName << std::endl;
					firstColEdit = true;
					layerManager_.SaveToFile();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Delete##Col", ImVec2(100, 0))) {
				if (layerManager_.RemoveCollisionLayer(layer.id)) {
					Logger::Get().Info(DebugType::Editor)
						<< "Deleted collision layer" << std::endl;
					selectedCollisionLayerIdx_ = -1;
					firstColEdit = true;
					layerManager_.SaveToFile();
				}
			}

			if (!canEdit) {
				ImGui::EndDisabled();
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
					"Built-in layers cannot be edited or deleted");
			}
		}
	}

	// Helper function to show collision matrix
	void Editor::ShowCollisionMatrix(PhysicsSystem* physics) {

		// Get actual collision layers from LayerManager instead of hardcoded enum
		const auto& collisionLayers = layerManager_.GetCollisionLayers();
		int layerCount = static_cast<int>(collisionLayers.size());

		const float ROW_HEADER_WIDTH = 100.0f;
		const float CELL_SIZE = 30.0f;

		ImVec2 headerStartPos = ImGui::GetCursorScreenPos();
		ImGui::NewLine();

		// Draw Matrix Grid using actual layers
		for (int row = 0; row < layerCount; row++) {
			const auto& rowLayer = collisionLayers[row];
			Framework::CollisionLayer rowLayerEnum = static_cast<Framework::CollisionLayer>(rowLayer.id);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", rowLayer.name.c_str()); //  Use layer name
			ImGui::SameLine(ROW_HEADER_WIDTH);

			for (int col = 0; col < layerCount; col++) {
				const auto& colLayer = collisionLayers[col];
				Framework::CollisionLayer colLayerEnum = static_cast<Framework::CollisionLayer>(colLayer.id);

				ImGui::PushID(row * 32 + col); // Use 32 instead of layerCount for unique IDs
				bool canCollide = physics->GetLayerCollision(rowLayerEnum, colLayerEnum);

				ImGui::SetCursorPosX(ROW_HEADER_WIDTH + (col * CELL_SIZE) +
					(CELL_SIZE - ImGui::GetFrameHeight()) * 0.5f);

				if (ImGui::Checkbox("##cb", &canCollide)) {
					//layerManager_.SetLayerCollision(rowLayer.id, colLayer.id, canCollide);
					physics->SetLayerCollision(rowLayerEnum, colLayerEnum, canCollide);

					Logger::Get().Info(DebugType::Editor)
						<< "Collision changed: " << rowLayer.name << " vs " << colLayer.name
						<< " = " << (canCollide ? "ENABLED" : "DISABLED") << std::endl;

					//LayerManager& layerMgr = gameManager_->GetLayerManager();
					/*layerMgr.SetLayerCollision(rowLayer.id, colLayer.id, canCollide);
					layerMgr.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH);*/
					//layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH, physics);

					if (layerManager_.SaveToFile(FileIOKeys::LAYER_CONFIG_FILEPATH, physics)) {
						Logger::Get().Info(DebugType::Editor)
							<< "Collision matrix saved to file" << std::endl;
					}
					else {
						Logger::Get().Error(DebugType::Editor)
							<< "FAILED to save collision matrix!" << std::endl;
					}
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s vs %s", rowLayer.name.c_str(), colLayer.name.c_str());
				}

				ImGui::SameLine();
				ImGui::PopID();
			}
			ImGui::NewLine();
		}
		
	}

	// Helper function to show entity collision layer
	void Editor::ShowEntityCollisionLayer(Framework::EntityManager& entityMgr, Entity selectedEntity) {

		EntityStorage& ecs = GameManager::GetInstance().GetECS();
		Framework::CollisionLayer& entityLayer = entityMgr.GetProperties(selectedEntity).layer;

		const std::vector<Framework::CollisionLayerData>& collisionLayers = layerManager_.GetCollisionLayers();

		// Find current layer index
		int currentIdx = -1;
		for (size_t i = 0; i < collisionLayers.size(); ++i) {
			if (collisionLayers[i].id == static_cast<uint8_t>(entityLayer)) {
				currentIdx = static_cast<int>(i);
				break;
			}
		}

		// Build dropdown
		std::vector<std::string> layerNames;
		std::vector<const char*> layerItems;

		for (const Framework::CollisionLayerData& layer : collisionLayers) {
			layerNames.push_back(layer.name);
		}

		for (const std::string& name : layerNames) {
			layerItems.push_back(name.c_str());
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Collision Layer:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);

		if (ImGui::Combo("##CollisionLayer", &currentIdx, layerItems.data(), static_cast<int>(layerItems.size()))) {
			if (currentIdx >= 0 && currentIdx < static_cast<int>(collisionLayers.size())) {
				std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);
				
				entityLayer = static_cast<Framework::CollisionLayer>(collisionLayers[currentIdx].id);
				entityMgr.GetProperties(selectedEntity).collisionLayerName = collisionLayers[currentIdx].name;

				std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified Collision Layer in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
				}
			}
		}

		ImGui::Spacing();
		ImGui::TextDisabled("This determines which objects can collide with this entity");

		// Show collision interactions
		ImGui::Spacing();
		if (ImGui::TreeNode("Collision Interactions")) {
			ImGui::Text("Can collide with:");
			ImGui::Indent();

			bool hasCollisions = false;
			auto* physics = gameManager_->GetSystems().physics_;

			for (const auto& otherLayer : collisionLayers) {
				Framework::CollisionLayer other = static_cast<Framework::CollisionLayer>(otherLayer.id);
				if (physics->GetLayerCollision(entityLayer, other)) {
					ImGui::BulletText("%s", otherLayer.name.c_str());
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

		EntityStorage& ecs = GameManager::GetInstance().GetECS();
		Framework::EntityProperties& props = entityMgr.GetProperties(selectedEntity);

		const std::vector<RenderLayerData>& renderLayers = layerManager_.GetRenderLayers();

		// Build dropdown with custom option
		std::vector<const char*> layerItems;
		layerItems.push_back("Custom");
		for (const RenderLayerData& layer : renderLayers) {
			layerItems.push_back(layer.name.c_str());
		}

		// Determine current selection
		int currentIdx = 0; // Default to Custom

		if (props.renderLayer != RenderLayer::Custom) {
			// Try to find matching layer by render order
			for (size_t i = 0; i < renderLayers.size(); ++i) {
				if (renderLayers[i].renderOrder == static_cast<int>(props.renderLayer)) {
					currentIdx = static_cast<int>(i) + 1; // +1 because Custom is at 0
					break;
				}
			}
		}
		else {
			currentIdx = 0;
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Render Layer:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);

		if (ImGui::Combo("##RenderLayer", &currentIdx, layerItems.data(), static_cast<int>(layerItems.size()))) {

			std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

			if (currentIdx == 0) {
				// Custom selected
				props.renderLayer = Framework::RenderLayer::Custom;
				props.renderLayerName = "Custom";
			}
			else {
				// Predefined layer selected
				int layerIdx = currentIdx - 1;
				if (layerIdx >= 0 && layerIdx < static_cast<int>(renderLayers.size())) {
					props.customRenderOrder = renderLayers[layerIdx].renderOrder;
					props.renderLayer = static_cast<RenderLayer>(renderLayers[layerIdx].renderOrder);
					props.renderLayerName = renderLayers[layerIdx].name; // Store the layer name
				}
			}

			std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

			if (beforeJSON != afterJSON) {
				std::stringstream ss;
				ss << "Modified Render Layer in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
				historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
			}
		}

		ImGui::Spacing();
		ImGui::TextDisabled("This controls when this entity is drawn relative to others");

		// Custom render order
		if (props.renderLayerName == "Custom" || props.renderLayerName.empty()) {
			static std::string beforeRenderOrderJSON;
			int newRenderOrder = props.customRenderOrder;

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Custom Render Order:");
			ImGui::SetNextItemWidth(150.0f);
			ImGui::DragInt("##CustomRenderOrder", &newRenderOrder, 1.0f, -1000, 1000);

			if (ImGui::IsItemActivated()) {
				beforeRenderOrderJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);
			}

			if (newRenderOrder != props.customRenderOrder) {
				props.customRenderOrder = newRenderOrder;
				props.renderLayer = RenderLayer::Custom;
				props.renderLayerName = "Custom"; // Mark as truly custom
			}

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				if (beforeRenderOrderJSON != afterJSON) {
					std::stringstream ss;
					ss << "Modified Custom Render Order in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeRenderOrderJSON, afterJSON);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset to 0")) {
				std::string beforeJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				props.customRenderOrder = 0;
				props.renderLayer = RenderLayer::Custom;
				props.renderLayerName = "Custom";

				std::string afterJSON = EntitySerializer::SerializeEntityPropertiesToString(selectedEntity, ecs);

				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Reset Custom Render Order in " << entityMgr.GetProperties(selectedEntity).name << std::endl;
					historyManager_.RecordModifyComponent(selectedEntity, ss.str(), beforeJSON, afterJSON);
				}
			}

			ImGui::Spacing();
			ImGui::TextWrapped("Lower values render first (behind). Higher values render last (in front).");
		}
		else {
			// Show which named layer is active
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
				"Using layer: %s (Order: %d)",
				props.renderLayerName.c_str(),
				props.customRenderOrder);
		}

		// Show render order info
		ImGui::Spacing();
		if (ImGui::TreeNode("Render Order Info")) {
			int effectiveOrder = props.customRenderOrder;

			ImGui::Text("Effective Render Order: %d", effectiveOrder);
			ImGui::Spacing();

			ImGui::TextWrapped("Standard render order:");
			ImGui::Indent();
			for (const auto& layer : renderLayers) {
				bool isCurrent = (layer.name == props.renderLayerName);
				if (isCurrent) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
						"► %s: %d (CURRENT)", layer.name.c_str(), layer.renderOrder);
				}
				else {
					ImGui::BulletText("%s: %d", layer.name.c_str(), layer.renderOrder);
				}
			}
			ImGui::Unindent();

			ImGui::TreePop();
		}
	}

	// Helper function to show scene entity layer tracker
	void Editor::ShowSceneEntityTracker(Framework::EntityManager& entityMgr) {
		Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
		const Framework::SceneAsset& currentScene = sceneMgr.GetActiveScene();

		if (!currentScene.IsValid()) {
			ImGui::TextDisabled("No active scene loaded");
			return;
		}

		//const std::vector<Entity>& sceneEntities = sceneMgr.GetEntitiesInScene(currentScene.name);
		std::vector<Entity> sceneEntities = sceneMgr.GetEntitiesInScene(currentScene.name);

		if (sceneEntities.empty()) {
			ImGui::TextDisabled("No entities in current scene");
			return;
		}

		ImGui::Text("Scene: %s", currentScene.name.c_str());
		ImGui::Text("Total Entities: %zu", sceneEntities.size());
		ImGui::Spacing();
		ImGui::Separator();

		std::vector<Entity> sortedEntities = sceneEntities;
		SortEntitiesByRenderLayer(sortedEntities, entityMgr);

		// Display in a table	
		if (ImGui::BeginTable("EntityLayerTable", 4,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
			ImVec2(0.0f, 300.0f))) {

			ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Render Layer", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("Collision", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupScrollFreeze(0, 1); // Freeze header row
			ImGui::TableHeadersRow();

			//  Iterate through entity list
			for (const Entity& entity : sortedEntities) {
				if (!entityMgr.ValidateEntity(entity)) continue;

				const Framework::EntityProperties& props = entityMgr.GetProperties(entity);

				int effectiveOrder = props.customRenderOrder;

				ImGui::PushID(static_cast<int>(entity));
				ImGui::TableNextRow();

				//  Entity Name 
				ImGui::TableNextColumn();

				// Highlight if this is the selected entity
				bool isSelected = (selectedEntityIndex > 0 &&
					static_cast<Entity>(selectedEntityIndex - 1) == entity);

				if (isSelected) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
				}

				if (ImGui::Selectable(props.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
					// Flush any pending edits before changing selection
					editSession.Flush();
					selectedEntityIndex = entity + 1;
					isEntitySelected_ = true;
					selectedComponentIndex = 0;
					selectedTagIndex = 0;
				}

				if (isSelected) {
					ImGui::PopStyleColor();
				}

				// Render Layer Name
				ImGui::TableNextColumn();
				if (!props.renderLayerName.empty() && props.renderLayerName != "Custom") {
					// Show the named layer
					ImGui::Text("%s", props.renderLayerName.c_str());
				}
				else {
					// Fallback to showing "Custom"
					ImGui::Text("Custom");
				}

				//Effective Order
				ImGui::TableNextColumn();
				if (props.renderLayer == Framework::RenderLayer::Custom) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", effectiveOrder);
				}
				else {
					ImGui::Text("%d", effectiveOrder);
				}

				// Collision Layer 
				ImGui::TableNextColumn();
				if (!props.collisionLayerName.empty()) {
					// Show the named collision layer
					ImGui::Text("%s", props.collisionLayerName.c_str());
				}
				else {
					// Fallback to enum string
					ImGui::Text("%s", LayerToString(props.layer));
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::TextDisabled("Sorted by Render Order (Highest on Top). Click to select.");

	}

#pragma endregion

#pragma region MENUBAR

	void Editor::ShowMenuBar() {
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.05f, 0.05f, 0.05f, 1.f));

		if (ImGui::BeginMainMenuBar()) {
			// File Submenu Dropdown
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Create New Project...")) {
					// Reset buffers and open the modal next frame
					memset(newProjectNameBuffer_, 0, sizeof(newProjectNameBuffer_));
					memset(newProjectPathBuffer_, 0, sizeof(newProjectPathBuffer_));
					projectFeedbackMsg_.clear();
					// Default the root path to one level above cwd (typical repo layout)
					std::string defaultRoot = std::filesystem::current_path().parent_path().parent_path().string();
					strncpy_s(newProjectPathBuffer_, sizeof(newProjectPathBuffer_),
						defaultRoot.c_str(), _TRUNCATE);
					showNewProjectDialog_ = true;
				}

				if (ImGui::MenuItem("Import Existing Project...")) {
					memset(importProjectPathBuffer_, 0, sizeof(importProjectPathBuffer_));
					projectFeedbackMsg_.clear();
					showImportProjectDialog_ = true;
				}

				ImGui::EndMenu();
			}

			// Edit Submenu Dropdown
			if (ImGui::BeginMenu("Edit")) {
				EntityManager entityMgr(GameManager::GetInstance().GetECS());
				SceneManager& sceneMgr = SceneManager::GetInstance();
				const SceneAsset& activeScene = sceneMgr.GetActiveScene();

				// History Subsection
				ImGui::SeparatorText("HISTORY");
				if (ImGui::MenuItem("Undo", "Ctrl+Z", false, historyManager_.CanUndo())) {
					historyManager_.Undo();
				}
				if (ImGui::MenuItem("Redo", "Ctrl+Y", false, historyManager_.CanRedo())) {
					historyManager_.Redo();
				}
				// End History Subsection

				// Edit Subsection
				ImGui::SeparatorText("EDIT");
				if (ImGui::MenuItem("Copy", "Ctrl+C", false, isEntitySelected_)) {
					if (!selectedEntities_.empty()) {
						if (selectedEntities_.size() > 1) {
							CopyEntitiesToClipboard(selectedEntities_);
						}
						else {
							CopyEntityToClipboard(selectedEntities_[0]);
						}
					}
					else if (selectedEntityIndex > 0) {
						CopyEntityToClipboard(selectedEntityIndex - 1);
					}
				}
				if (ImGui::MenuItem("Paste", "Ctrl+V", false, !entitiesClipboard_.empty())) {
					PasteEntitiesFromClipboard();
				}
				if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, isEntitySelected_)) {
					Entity selectedEntity = selectedEntityIndex - 1;
					SceneManagement::SceneGUID ownerSceneID = activeScene.sceneIndex;
					if (entityMgr.HasComponent(selectedEntity, SceneOwnedType)) {
						ownerSceneID = entityMgr.GetComponent<SceneOwned>(selectedEntity).sceneIndex;
					}

					const SceneAsset& ownerScene = sceneMgr.GetSceneAt(ownerSceneID);
					Entity clonedEntity = entityMgr.CloneEntity(selectedEntity);
					sceneMgr.AddEntityToScene(ownerScene, clonedEntity);
					historyManager_.Record(OpType::Create, clonedEntity, "Duplicated Entity");
				}
				if (ImGui::MenuItem("Delete", "Delete", false, isEntitySelected_)) {
					Entity selectedEntity = selectedEntityIndex - 1;
					historyManager_.Record(OpType::Delete, selectedEntity, "Deleted Entity");
					entityMgr.DestroyEntity(selectedEntity);
					if (static_cast<unsigned int>((selectedEntityIndex - 1)) == selectedEntity) {
						selectedEntityIndex = selectedTagIndex = selectedComponentIndex = 0;
						isEntitySelected_ = false;
					}
				}
				// End Edit Subsection

				// Configuration Subsection
				ImGui::SeparatorText("CONFIGURATION");
				if (ImGui::MenuItem("Editor Preferences...")) {
					settings.showPreferencesWindow = true;
				}
				if (ImGui::MenuItem("Project Settings...")) {
					settings.showProjectSettingsWindow = true;
				}
				// End Configuration Subsection

				ImGui::EndMenu();
				// End Edit Submenu
			}

			// Window Submenu
			if (ImGui::BeginMenu("Window")) {
				ImGui::MenuItem("Inspector", NULL, &settings.showInspector);
				ImGui::MenuItem("Hierarchy", NULL, &settings.showHierarchy);
				ImGui::MenuItem("Level Editor", NULL, &settings.showLevelEditor);
				ImGui::MenuItem("LayerManager", NULL, &settings.showLayerManager);
				ImGui::EndMenu();
			}
			// End Window Submenu

			// Tools Submenu
			if (ImGui::BeginMenu("Tools")) {
				ImGui::MenuItem("FSM Enemy Editor", NULL, &isFSMEditorOpen_);
				ImGui::MenuItem("Prefab Editor", NULL, &isPrefabEditorOpen_);
				ImGui::MenuItem("Animation Editor", NULL, &settings.showAnimationEditor);
				
				bool isAnalyzerOpen = buildSizeAnalyzer_.IsOpen();
				if (ImGui::MenuItem("Build Size Analyzer", NULL, &isAnalyzerOpen)) {
					buildSizeAnalyzer_.SetOpen(isAnalyzerOpen);
				}

				if (ImGui::BeginMenu("Debug")) {
					ImGui::MenuItem("Show Grid", NULL, &showGrid_);
					Framework::SpriteRenderSystem* spriteRenderer = GameManager::GetInstance().GetSystems().spriteRenderer_;
					if (spriteRenderer) {
						bool showColliders = spriteRenderer->IsDrawingColliders();
						if (ImGui::MenuItem("Show Colliders", NULL, &showColliders)) {
							spriteRenderer->ToggleColliders();
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			// End Tools Submenu

			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleColor();

		// Render project management modals (they open via ImGui::OpenPopup below)
		if (showNewProjectDialog_)    ShowNewProjectDialog();
		if (showImportProjectDialog_) ShowImportProjectDialog();
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

			// Gizmos keyboard shortcuts
			if (gizmoSystem_) {
				ImVec4 selectedModeClr = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); // cyan
				ImVec4 greyClr = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // grey

				ImGui::TextColored((!isMarqueeMode_ && !gizmoSystem_->IsGizmosToggled()) ? selectedModeClr : greyClr, "Q - Select Mode");
				ImGui::SameLine(0, spacing);
				ImGui::TextColored((gizmoSystem_->currentMode_ == GizmoMode::Translate) ? selectedModeClr : greyClr, "T - Tranlate/Move Mode");
				ImGui::SameLine(0, spacing);
				ImGui::TextColored((gizmoSystem_->currentMode_ == GizmoMode::Rotate) ? selectedModeClr : greyClr, "Y - Rotate Mode");
				ImGui::SameLine(0, spacing);
				ImGui::TextColored((gizmoSystem_->currentMode_ == GizmoMode::Scale) ? selectedModeClr : greyClr, "U - Scale Mode");
				ImGui::SameLine(0, spacing);
				ImGui::TextColored(isMarqueeMode_ ? selectedModeClr : greyClr, "I - Marquee Mode");
			}

			ImGui::SameLine();

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

			// Lock game keyboard when not playing. When playing, let the game
			// manage the lock itself (e.g. LevelSelection locks keyboard for prompts).
			if (!isGamePlaying) {
				gameManager_->GetSystems().input_->SetKeyboardLocked(true);
			}

			if (ImGui::Button(isGamePlaying ? "Stop" : "Play", ImVec2(buttonWidth, buttonHeight)))
			{
				if (physics_->IsInStepMode()) physics_->ToggleStepMode();

				if (isGamePlaying) {
					gameManager_->OnEndPlay();
					ResetEditorCamera();
					gameManager_->GetSystems().input_->SetKeyboardLocked(true);
				}
				else {
					gameManager_->OnBeginPlay();
					gameManager_->GetSystems().input_->SetKeyboardLocked(false);
				}

				isGamePlaying = !isGamePlaying;
				isGamePaused = false;

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
			if (historyManager_.CanUndo() && !IsGamePlaying()) {
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
			if (historyManager_.CanRedo() && !IsGamePlaying()) {
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
		}
		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

#pragma endregion

#pragma region CONFIG WINDOWS

	void Editor::ShowPreferencesWindow() {
		ImGui::SetNextWindowSize(ImVec2(850, 550), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Editor Preferences", &settings.showPreferencesWindow)) {
			ImGui::SeparatorText("History");

			static int currentLimit = historyManager_.GetUndoLimit();

			ImGui::DragInt("Max History Steps", &currentLimit, 1.f, 1, 100);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				historyManager_.SetUndoLimit(currentLimit);
				SaveEditorConfig();
			}

			if (!ImGui::IsItemActive()) {
				currentLimit = historyManager_.GetUndoLimit();
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Maximum number of operations stored in the Undo/Redo Stack. Changing this value will wipe the current history stack.");
			}
		}
		ImGui::End();
	}

	void Editor::ShowProjectSettingsWindow() {
		ImGui::SetNextWindowSize(ImVec2(850, 550), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Project Settings", &settings.showProjectSettingsWindow)) {
			static int selectedCategory = 0;
			const char* categories[] = { "Game", "Physics 2D", "Performance" };

			// --- Left Pane (Sidebar) ---
			ImGui::BeginChild("Sidebar", ImVec2(220, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
			for (int i = 0; i < IM_ARRAYSIZE(categories); ++i) {
				if (ImGui::Selectable(categories[i], selectedCategory == i)) {
					selectedCategory = i;
				}
			}
			ImGui::EndChild();
			// --- Left Pane (Sidebar) ---

			// --- Right Pane (Config) ---
			ImGui::SameLine();
			ImGui::BeginChild("Configuration", ImVec2(0, 0), false);

			// Header
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
			ImGui::Text("%s", categories[selectedCategory]);
			ImGui::PopFont();
			ImGui::Separator();
			ImGui::Spacing();

			// Body
			switch (selectedCategory) {
			case 0: { // Game Settings
				GameManager& gm = GameManager::GetInstance();
				SceneManager& sceneMgr = SceneManager::GetInstance();
				const auto& allScenes = sceneMgr.GetAllSceneAssets();

				// Collect scene names (excluding reserved scenes)
				static std::vector<std::string> sceneNames;
				sceneNames.clear();
				sceneNames.push_back("(none)");
				for (const auto& [id, asset] : allScenes) {
					if (asset.IsValid() &&
						id != SceneManager::GetDDOLSceneID() &&
						id != SceneManager::GetInvalidSceneID()) {
						sceneNames.push_back(asset.name);
					}
				}
				// Sort for stable display
				std::sort(sceneNames.begin() + 1, sceneNames.end());

				GameManager::GameConfig& cfg = gm.GetConfig();

				auto FindIdx = [&](const std::string& val) -> int {
					for (int i = 0; i < (int)sceneNames.size(); ++i)
						if (sceneNames[i] == val) return i;
					return 0;
				};

				ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;
				if (ImGui::BeginTable("GameConfigProperties", 2, tableFlags)) {
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 200.f);
					ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

					// Startup Scene
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Startup Scene");
					ImGui::TableSetColumnIndex(1);
					{
						int startupIdx = FindIdx(cfg.startupScene);
						std::vector<const char*> items;
						for (const auto& s : sceneNames) items.push_back(s.c_str());
						ImGui::PushItemWidth(-FLT_MIN);
						if (ImGui::Combo("##Startup", &startupIdx, items.data(), (int)items.size()))
							cfg.startupScene = (startupIdx == 0) ? "" : sceneNames[startupIdx];
						ImGui::PopItemWidth();
					}

					// Loading Scene
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Loading Scene");
					ImGui::TableSetColumnIndex(1);
					{
						int loadingIdx = FindIdx(cfg.loadingScene);
						std::vector<const char*> items;
						for (const auto& s : sceneNames) items.push_back(s.c_str());
						ImGui::PushItemWidth(-FLT_MIN);
						if (ImGui::Combo("##Loading", &loadingIdx, items.data(), (int)items.size()))
							cfg.loadingScene = (loadingIdx == 0) ? "" : sceneNames[loadingIdx];
						ImGui::PopItemWidth();
					}

					ImGui::EndTable();
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::TextUnformatted("Scene Tags");
				ImGui::Spacing();

				// Per-scene checkboxes for menu / overlay categories
				if (ImGui::BeginTable("SceneTagTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
					ImGui::TableSetupColumn("Scene Name", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Menu Scene", ImGuiTableColumnFlags_WidthFixed, 100.f);
					ImGui::TableSetupColumn("Overlay Scene", ImGuiTableColumnFlags_WidthFixed, 110.f);
					ImGui::TableHeadersRow();

					// Skip "(none)" at index 0
					for (int i = 1; i < (int)sceneNames.size(); ++i) {
						const std::string& name = sceneNames[i];
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::TextUnformatted(name.c_str());

						auto Contains = [](const std::vector<std::string>& vec, const std::string& val) {
							return std::find(vec.begin(), vec.end(), val) != vec.end();
						};
						auto Toggle = [](std::vector<std::string>& vec, const std::string& val, bool add) {
							auto it = std::find(vec.begin(), vec.end(), val);
							if (add && it == vec.end()) vec.push_back(val);
							else if (!add && it != vec.end()) vec.erase(it);
						};

						ImGui::TableSetColumnIndex(1);
						bool isMenu = Contains(cfg.menuScenes, name);
						ImGui::PushID(("menu_" + name).c_str());
						if (ImGui::Checkbox("##m", &isMenu)) Toggle(cfg.menuScenes, name, isMenu);
						ImGui::PopID();

						ImGui::TableSetColumnIndex(2);
						bool isOverlay = Contains(cfg.overlayScenes, name);
						ImGui::PushID(("overlay_" + name).c_str());
						if (ImGui::Checkbox("##o", &isOverlay)) Toggle(cfg.overlayScenes, name, isOverlay);
						ImGui::PopID();
					}
					ImGui::EndTable();
				}

				ImGui::Spacing();
				if (ImGui::Button("Save Game Config")) {
					FileIOSystem::SaveGameConfig(cfg);
				}
				break;
			}
			case 1: { // Physics2D
				Framework::PhysicsSystem& physics = *GameManager::GetInstance().GetSystems().physics_;

				ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;
				if (ImGui::BeginTable("PhysicsProperties", 2, tableFlags)) {
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 200.f);
					ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

					// Gravity
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("World Gravity");

					ImGui::TableSetColumnIndex(1);
					ImGui::PushItemWidth(-FLT_MIN);
					if (ImGui::DragFloat2("##Gravity", &physics.gravity_.x, 0.1f)) {
						Framework::FileIOSystem::SaveWorldConfig();
					}
					ImGui::PopItemWidth();
					ImGui::EndTable();
				}
				break;
			}
			case 2: { // Performance
				Framework::Graphics& gfx = *GameManager::GetInstance().GetSystems().gfx_;

				ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;
				if (ImGui::BeginTable("PerformanceProperties", 2, tableFlags)) {
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 200.f);
					ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

					bool enableFPSLimit = gfx.IsFPSCapEnabled();
					static int fpsLimit = static_cast<int>(gfx.GetFPSCap());

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Enable FPS Limit");

					ImGui::TableSetColumnIndex(1);
					if (ImGui::Checkbox("##EnableFPS", &enableFPSLimit)) {
						gfx.ToggleFPSCap();
						Framework::FileIOSystem::SaveWorldConfig();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Max FPS");

					ImGui::TableSetColumnIndex(1);
					if (!enableFPSLimit) ImGui::BeginDisabled();
					ImGui::PushItemWidth(-FLT_MIN);

					ImGui::DragInt("##MaxFPS", &fpsLimit, 1.f, 15, 240);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						gfx.SetFPSCap(static_cast<float>(fpsLimit));
						Framework::FileIOSystem::SaveWorldConfig();
					}

					if (!ImGui::IsItemActive()) {
						fpsLimit = static_cast<int>(gfx.GetFPSCap());
					}

					ImGui::PopItemWidth();
					if (!enableFPSLimit) ImGui::EndDisabled();
					ImGui::EndTable();
				}
				break;
			}
			default: break;
			}
			ImGui::EndChild();
			// -- Right Pane (Config) ---
		}
		ImGui::End();
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
		const float cell = normalized ? 0.1f : baseGridCellSize_/2; // world units
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

	void Editor::ShowCreatePrefabDialog() {
		using namespace Framework;

		if (!showCreatePrefabDialog_) return;

		if (!ImGui::IsPopupOpen("Create Custom Prefab")) {
			ImGui::OpenPopup("Create Custom Prefab");
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("Create Custom Prefab", &showCreatePrefabDialog_, ImGuiWindowFlags_NoResize)) {
			ImGui::TextWrapped("Create a custom prefab from the selected entity.");
			ImGui::TextWrapped("This will save to: Data/Prefabs/Custom/");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Prefab Name:");
			ImGui::InputText("##NewPrefabName", newPrefabNameBuffer_, sizeof(newPrefabNameBuffer_));

			ImGui::Spacing();

			if (entityForPrefabCreation_ != 0) {
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));
				if (entityMgr.ValidateEntity(entityForPrefabCreation_)) {
					EntityProperties& props = entityMgr.GetProperties(entityForPrefabCreation_);
					ImGui::Text("Source Entity: %s", props.name.c_str());
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			bool canCreate = strlen(newPrefabNameBuffer_) > 0;

			if (!canCreate) ImGui::BeginDisabled();

			if (ImGui::Button("Create Prefab", ImVec2(120, 0))) {
				std::string prefabName(newPrefabNameBuffer_);
				if (prefabSystem_.SaveEntityAsPrefab(*(gameManager_->GetSystems().ecs_),
					entityForPrefabCreation_, prefabName, "Custom")) {
					showCreatePrefabDialog_ = false;
					memset(newPrefabNameBuffer_, 0, sizeof(newPrefabNameBuffer_));
					palettePrepared_ = false;
					ImGui::CloseCurrentPopup();
				}
			}

			if (!canCreate) ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				showCreatePrefabDialog_ = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		else {
			showCreatePrefabDialog_ = false;
		}
	}

	void Editor::ShowPrefabEditor() {
		using namespace Framework;
		ImGui::Begin("Prefab Editor", &isPrefabEditorOpen_);
		ImGui::SeparatorText("Prefab Actions");
		{
			bool hasSelectedEntity = isEntitySelected_ && selectedEntityIndex > 0;
			Entity selectedEntity = hasSelectedEntity ? static_cast<Entity>(selectedEntityIndex - 1) : 0;

			if (!hasSelectedEntity) {
				ImGui::BeginDisabled();
			}

			if (ImGui::Button("Create Prefab from Selected Entity", ImVec2(300, 30))) {
				EntityManager entityMgr(*(gameManager_->GetSystems().ecs_));

				if (entityMgr.ValidateEntity(selectedEntity)) {
					showCreatePrefabDialog_ = true;
					entityForPrefabCreation_ = selectedEntity;
					EntityProperties& props = entityMgr.GetProperties(selectedEntity);
					strncpy_s(newPrefabNameBuffer_, sizeof(newPrefabNameBuffer_), props.name.c_str(), _TRUNCATE);
				}
			}

			if (!hasSelectedEntity) {
				ImGui::EndDisabled();
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				if (hasSelectedEntity) {
					ImGui::SetTooltip("Save the selected entity as a custom prefab");
				}
				else {
					ImGui::SetTooltip("Please select an entity in the scene first");
				}
			}
		}
		ImGui::SameLine();
		{
			bool canDelete = !selectedPrefabForEditing_.empty();
			if (canDelete) {
				const EntityPrefab* prefab = prefabSystem_.GetPrefab(selectedPrefabForEditing_);
				if (!prefab) {
					canDelete = false;
				}
			}
			if (!canDelete) {
				ImGui::BeginDisabled();
			}
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button("Delete Selected Prefab", ImVec2(200, 30))) {
				ImGui::OpenPopup("Confirm Delete Prefab");
			}

			ImGui::PopStyleColor(3);

			if (!canDelete) {
				ImGui::EndDisabled();
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				if (canDelete) {
					ImGui::SetTooltip("Delete this prefab");
				}
				else {
					ImGui::SetTooltip("Please select a prefab first");
				}
			}
			if (ImGui::BeginPopupModal("Confirm Delete Prefab", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Are you sure you want to delete the prefab:");
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", selectedPrefabForEditing_.c_str());
				ImGui::Text("This action cannot be undone!");

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::Button("Delete", ImVec2(120, 0))) {
					if (prefabSystem_.DeletePrefab(selectedPrefabForEditing_)) {
						Logger::Get().Info(DebugType::Editor)
							<< "Deleted custom prefab: " << selectedPrefabForEditing_ << std::endl;

						selectedPrefabForEditing_.clear();
						selectedPrefabTypeForEditing_ = PrefabType::None;
						palettePrepared_ = false;
					}
					else {
						Logger::Get().Error(DebugType::Editor)
							<< "Failed to delete prefab" << std::endl;
					}
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::End();
		using namespace Framework;

		ImGui::Begin("Prefab Editor", &isPrefabEditorOpen_);

		// Prefab Selection Dropdown
		ImGui::Text("Select Prefab to Edit:");

		static int selectedPrefabIndex = 0;
		static char searchBuffer[256] = "";
		static bool useFilteredList = false;
		static std::vector<std::string> filteredPrefabNames;

		std::vector<std::string> allPrefabNames;
		std::vector<const char*> prefabItems;

		// Search input
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
		if (ImGui::InputTextWithHint("##PrefabSearch", "Search prefabs...", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
			useFilteredList = (strlen(searchBuffer) > 0);
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			searchBuffer[0] = '\0';
			useFilteredList = false;
			selectedPrefabIndex = 0;
		}

		// Build the list of prefabs
		allPrefabNames.push_back("-- Select Prefab --");
		for (const auto& pair : prefabSystem_.GetAllPrefabs()) {
			allPrefabNames.push_back(pair.first);
		}

		// Apply search filter if needed
		if (useFilteredList && strlen(searchBuffer) > 0) {
			filteredPrefabNames.clear();
			filteredPrefabNames.push_back("-- Select Prefab --");

			std::string searchLower = searchBuffer;
			std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(),
				[](unsigned char c) { return static_cast<char>(::tolower(c)); });

			for (const auto& name : allPrefabNames) {
				if (name == "-- Select Prefab --") continue;

				std::string nameLower = name;
				std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
					[](unsigned char c) { return static_cast<char>(::tolower(c)); });

				if (nameLower.find(searchLower) != std::string::npos) {
					filteredPrefabNames.push_back(name);
				}
			}

			// Update selected index if current selection is not in filtered list
			if (selectedPrefabIndex > 0 && selectedPrefabIndex < allPrefabNames.size()) {
				std::string currentSelection = allPrefabNames[selectedPrefabIndex];
				auto it = std::find(filteredPrefabNames.begin(), filteredPrefabNames.end(), currentSelection);
				if (it == filteredPrefabNames.end()) {
					selectedPrefabIndex = 0;
				}
				else {
					selectedPrefabIndex = static_cast<int>(std::distance(filteredPrefabNames.begin(), it));
				}
			}

			// Use filtered list
			const auto& displayNames = filteredPrefabNames;
			for (const auto& name : displayNames) {
				prefabItems.push_back(name.c_str());
			}

			// Show count
			ImGui::TextDisabled("Showing %d/%d prefabs", filteredPrefabNames.size() - 1, allPrefabNames.size() - 1);
		}
		else {
			// Use full list
			const auto& displayNames = allPrefabNames;
			for (const auto& name : displayNames) {
				prefabItems.push_back(name.c_str());
			}
		}

		// Combo box
		if (ImGui::Combo("##PrefabList", &selectedPrefabIndex, prefabItems.data(), static_cast<int>(prefabItems.size()))) {
			std::string selectedName;
			if (useFilteredList && !filteredPrefabNames.empty()) {
				selectedName = (selectedPrefabIndex >= 0 && selectedPrefabIndex < filteredPrefabNames.size())
					? filteredPrefabNames[selectedPrefabIndex] : "";
			}
			else {
				selectedName = (selectedPrefabIndex >= 0 && selectedPrefabIndex < allPrefabNames.size())
					? allPrefabNames[selectedPrefabIndex] : "";
			}

			if (selectedPrefabIndex > 0 && !selectedName.empty()) {
				selectedPrefabForEditing_ = selectedName;

				// Determine prefab type by checking components
				EntityPrefab* prefab = prefabSystem_.GetMutablePrefab(selectedPrefabForEditing_);
				if (prefab) {
					bool hasTrap = false;

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

			// === COMPONENT EDITOR ===
			ImGui::SeparatorText("Components");

			for (auto& compData : prefab->components) {

				if (ImGui::CollapsingHeader(compData.type.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

					// === TRANSFORM COMPONENT ===
					if (compData.type == "Transform") {
					
						// Add Rotation Section
						if (compData.data.HasMember("Rotation") && compData.data["Rotation"].IsObject()) {
							auto& rot = compData.data["Rotation"];

							ImGui::Text("Rotation:");

							// Texture Angle
							if (rot.HasMember("texture_angle")) {
								float textureAngleDeg = static_cast<float>(rot["texture_angle"].GetFloat() * (180.0f / M_PI));;
								if (ImGui::DragFloat("Texture Angle (degrees)", &textureAngleDeg, 1.0f, -180.0f, 180.0f)) {
									rot["texture_angle"].SetFloat(static_cast<float>(textureAngleDeg* (M_PI / 180.0f)));
									prefabModified = true;
								}
							}

							// Collider Angle
							if (rot.HasMember("collider_angle")) {
								float colliderAngleDeg = static_cast<float>(rot["collider_angle"].GetFloat() * (180.0f / M_PI));
								if (ImGui::DragFloat("Collider Angle (degrees)", &colliderAngleDeg, 1.0f, -180.0f, 180.0f)) {
									rot["collider_angle"].SetFloat(static_cast<float>(colliderAngleDeg* (M_PI / 180.0f)));
									prefabModified = true;
								}
							}

							// Quick reset button
							if (ImGui::Button("Reset Rotation")) {
								if (rot.HasMember("texture_angle")) rot["texture_angle"].SetFloat(0.0f);
								if (rot.HasMember("collider_angle")) rot["collider_angle"].SetFloat(0.0f);
								if (rot.HasMember("angle")) rot["angle"].SetFloat(0.0f);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("Scale") && compData.data["Scale"].IsObject()) {
							auto& scale = compData.data["Scale"];
							float scaleValues[2] = { scale["x"].GetFloat(), scale["y"].GetFloat() };

							if (ImGui::DragFloat2("Scale", scaleValues, 0.1f, 0.1f, 10.0f)) {
								scale["x"].SetFloat(scaleValues[0]);
								scale["y"].SetFloat(scaleValues[1]);
								prefabModified = true;
							}
						}
					}
					// === RIGIDBODY COMPONENT ===
					else if (compData.type == "RigidBody") {
						if (compData.data.HasMember("mass")) {
							float mass = compData.data["mass"].GetFloat();
							if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 1.0f)) {
								compData.data["mass"].SetFloat(mass);
								prefabModified = true;
							}
						}
					}

					// === AABBCOLLIDER COMPONENT ===
					else if (compData.type == "AABBCollider") {
						if (compData.data.HasMember("half_width") && compData.data.HasMember("half_height")) {
							float halfX = compData.data["half_width"].GetFloat();
							float halfY = compData.data["half_height"].GetFloat();

							if (ImGui::DragFloat("Collider Half Width", &halfX, 1.0f, 0.0f, 10000.0f)) {
								compData.data["half_width"].SetFloat(halfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Collider Half Height", &halfY, 1.0f, 0.0f, 10000.0f)) {
								compData.data["half_height"].SetFloat(halfY);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("offset_x") && compData.data.HasMember("offset_y")) {
							float offsetX = compData.data["offset_x"].GetFloat();
							float offsetY = compData.data["offset_y"].GetFloat();

							if (ImGui::DragFloat("Offset X", &offsetX, 1.0f, -1000.0f, 1000.0f)) {
								compData.data["offset_x"].SetFloat(offsetX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Offset Y", &offsetY, 1.0f, -1000.0f, 1000.0f)) {
								compData.data["offset_y"].SetFloat(offsetY);
								prefabModified = true;
							}
						}
					}

					// === TEXTURECOMPONENT ===
					else if (compData.type == "TextureComponent") {
						if (compData.data.HasMember("texture_key")) {
							static char textureKeyBuffer[256] = "";
							std::string currentKey = compData.data["texture_key"].GetString();
							strncpy(textureKeyBuffer, currentKey.c_str(), 255);

							if (ImGui::InputText("Texture Key", textureKeyBuffer, 256)) {
								rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();
								compData.data["texture_key"].SetString(textureKeyBuffer, alloc);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("half_size") && compData.data["half_size"].IsObject()) {
							auto& halfSize = compData.data["half_size"];
							float texHalfX = halfSize["x"].GetFloat();
							float texHalfY = halfSize["y"].GetFloat();

							if (ImGui::DragFloat("Texture Half Size X", &texHalfX, 1.0f, 0.0f, 10000.0f)) {
								halfSize["x"].SetFloat(texHalfX);
								prefabModified = true;
							}
							if (ImGui::DragFloat("Texture Half Size Y", &texHalfY, 1.0f, 0.0f, 10000.0f)) {
								halfSize["y"].SetFloat(texHalfY);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("flip_x")) {
							bool flipX = compData.data["flip_x"].GetBool();
							if (ImGui::Checkbox("Flip Horizontal", &flipX)) {
								compData.data["flip_x"].SetBool(flipX);
								prefabModified = true;
							}
						}
					}

					// === VELOCITY COMPONENT ===
					else if (compData.type == "Velocity") {
						if (compData.data.HasMember("x") && compData.data.HasMember("y")) {
							float velValues[2] = { compData.data["x"].GetFloat(), compData.data["y"].GetFloat() };

							if (ImGui::DragFloat2("Velocity", velValues, 1.0f)) {
								compData.data["x"].SetFloat(velValues[0]);
								compData.data["y"].SetFloat(velValues[1]);
								prefabModified = true;
							}
						}
}

					// === ACCELERATION COMPONENT ===
					else if (compData.type == "Acceleration") {
						if (compData.data.HasMember("x") && compData.data.HasMember("y")) {
							float accelValues[2] = { compData.data["x"].GetFloat(), compData.data["y"].GetFloat() };

							if (ImGui::DragFloat2("Acceleration", accelValues, 1.0f)) {
								compData.data["x"].SetFloat(accelValues[0]);
								compData.data["y"].SetFloat(accelValues[1]);
								prefabModified = true;
							}
						}
					}
						

					// === ROTATIONFLAG COMPONENT ===
					else if (compData.type == "RotationFlag") {
						if (compData.data.HasMember("is_rotating")) {
							bool isRotating = compData.data["is_rotating"].GetBool();
							if (ImGui::Checkbox("Is Rotating", &isRotating)) {
								compData.data["is_rotating"].SetBool(isRotating);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("target_rotation")) {
							float targetRot = compData.data["target_rotation"].GetFloat();
							if (ImGui::DragFloat("Target Rotation", &targetRot, 1.0f, -360.0f, 360.0f)) {
								compData.data["target_rotation"].SetFloat(targetRot);
								prefabModified = true;
							}
						}
					}

					// === TRAPPLATFORM COMPONENT ===
					else if (compData.type == "TrapPlatformComponent") {
						if (compData.data.HasMember("fall_speed")) {
							float fallSpeed = compData.data["fall_speed"].GetFloat();
							if (ImGui::DragFloat("Fall Speed", &fallSpeed, 1.0f, 0.0f, 10000.0f)) {
								compData.data["fall_speed"].SetFloat(fallSpeed);
								prefabModified = true;
							}
						}
						if (compData.data.HasMember("trigger_delay")) {
							float triggerDelay = compData.data["trigger_delay"].GetFloat();
							if (ImGui::DragFloat("Trigger Delay", &triggerDelay, 0.1f, 0.0f, 5.0f)) {
								compData.data["trigger_delay"].SetFloat(triggerDelay);
								prefabModified = true;
							}
						}
					}

					// === AUDIOCOMPONENT ===
					else if (compData.type == "AudioComponent") {
						if (compData.data.HasMember("sound_key")) {
							static char soundKeyBuffer[256] = "";
							std::string currentKey = compData.data["sound_key"].GetString();
							strncpy(soundKeyBuffer, currentKey.c_str(), 255);

							if (ImGui::InputText("Sound Key", soundKeyBuffer, 256)) {
								rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();
								compData.data["sound_key"].SetString(soundKeyBuffer, alloc);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("should_loop")) {
							bool loop = compData.data["should_loop"].GetBool();
							if (ImGui::Checkbox("Loop", &loop)) {
								compData.data["should_loop"].SetBool(loop);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("volume")) {
							float volume = compData.data["volume"].GetFloat();
							if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
								compData.data["volume"].SetFloat(volume);
								prefabModified = true;
							}
						}
					}

					// === ANIMATIONCOMPONENT ===
					else if (compData.type == "AnimationComponent") {
						if (compData.data.HasMember("profile_name")) {
							static char profileBuffer[256] = "";
							std::string currentProfile = compData.data["profile_name"].GetString();
							strncpy(profileBuffer, currentProfile.c_str(), 255);

							if (ImGui::InputText("Animation Profile", profileBuffer, 256)) {
								rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();
								compData.data["profile_name"].SetString(profileBuffer, alloc);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("current_state")) {
							int currentState = compData.data["current_state"].GetInt();
							if (ImGui::DragInt("Current State", &currentState, 1, 0, 10)) {
								compData.data["current_state"].SetInt(currentState);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("current_frame")) {
							int currentFrame = compData.data["current_frame"].GetInt();
							if (ImGui::DragInt("Current Frame", &currentFrame, 1, 0, 100)) {
								compData.data["current_frame"].SetInt(currentFrame);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("frame_duration")) {
							float frameDuration = compData.data["frame_duration"].GetFloat();
							if (ImGui::DragFloat("Frame Duration", &frameDuration, 0.01f, 0.01f, 1.0f)) {
								compData.data["frame_duration"].SetFloat(frameDuration);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("start_frame")) {
							int startFrame = compData.data["start_frame"].GetInt();
							if (ImGui::DragInt("Start Frame", &startFrame, 1, 0, 100)) {
								compData.data["start_frame"].SetInt(startFrame);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("frame_count")) {
							int frameCount = compData.data["frame_count"].GetInt();
							if (ImGui::DragInt("Frame Count", &frameCount, 1, 1, 100)) {
								compData.data["frame_count"].SetInt(frameCount);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("elapsed_time")) {
							float elapsedTime = compData.data["elapsed_time"].GetFloat();
							if (ImGui::DragFloat("Elapsed Time", &elapsedTime, 0.01f, 0.0f, 10.0f)) {
								compData.data["elapsed_time"].SetFloat(elapsedTime);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("is_playing")) {
							bool isPlaying = compData.data["is_playing"].GetBool();
							if (ImGui::Checkbox("Is Playing", &isPlaying)) {
								compData.data["is_playing"].SetBool(isPlaying);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("is_looping")) {
							bool isLooping = compData.data["is_looping"].GetBool();
							if (ImGui::Checkbox("Is Looping", &isLooping)) {
								compData.data["is_looping"].SetBool(isLooping);
								prefabModified = true;
							}
						}

						// Start delay controls
						{
							rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();

							float startDelay = compData.data.HasMember("start_delay") ? compData.data["start_delay"].GetFloat() : 0.0f;
							if (ImGui::DragFloat("Start Delay (s)", &startDelay, 0.01f, 0.0f, 60.0f)) {
								if (compData.data.HasMember("start_delay"))
									compData.data["start_delay"].SetFloat(startDelay);
								else
									compData.data.AddMember("start_delay", startDelay, alloc);
								prefabModified = true;
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Seconds to wait before the animation begins playing");

							float startDelayRange = compData.data.HasMember("start_delay_range") ? compData.data["start_delay_range"].GetFloat() : 0.0f;
							if (ImGui::DragFloat("Delay Range (s)", &startDelayRange, 0.01f, 0.0f, 60.0f)) {
								if (compData.data.HasMember("start_delay_range"))
									compData.data["start_delay_range"].SetFloat(startDelayRange);
								else
									compData.data.AddMember("start_delay_range", startDelayRange, alloc);
								prefabModified = true;
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Randomises an extra delay in [0, range] on top of the base delay");
						}
					}
					// === TEXT COMPONENT ===
					else if (compData.type == "TextComponent") {
						if (compData.data.HasMember("text")) {
							static char textBuffer[256] = "";
							std::string currentText = compData.data["text"].GetString();
							strncpy(textBuffer, currentText.c_str(), 255);

							if (ImGui::InputText("Text", textBuffer, 256)) {
								rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();
								compData.data["text"].SetString(textBuffer, alloc);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("font_key")) {
							static char fontBuffer[256] = "";
							std::string currentFont = compData.data["font_key"].GetString();
							strncpy(fontBuffer, currentFont.c_str(), 255);

							if (ImGui::InputText("Font Key", fontBuffer, 256)) {
								rapidjson::Document::AllocatorType& alloc = compData.data.GetAllocator();
								compData.data["font_key"].SetString(fontBuffer, alloc);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("font_size")) {
							float fontSize = compData.data["font_size"].GetFloat();
							if (ImGui::DragFloat("Font Size", &fontSize, 0.1f, 0.1f, 10.0f)) {
								compData.data["font_size"].SetFloat(fontSize);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("max_width")) {
							float maxWidth = compData.data["max_width"].GetFloat();
							if (ImGui::DragFloat("Max Width", &maxWidth, 1.0f, 0.0f, 1000.0f)) {
								compData.data["max_width"].SetFloat(maxWidth);
								prefabModified = true;
							}
						}

						// Color editing
						if (compData.data.HasMember("font_color") && compData.data["font_color"].IsArray()) {
							auto& colorArray = compData.data["font_color"];
							if (colorArray.Size() >= 4) {
								float color[4] = {
									colorArray[0].GetFloat(),
									colorArray[1].GetFloat(),
									colorArray[2].GetFloat(),
									colorArray[3].GetFloat()
								};

								if (ImGui::ColorEdit4("Font Color", color)) {
									colorArray[0].SetFloat(color[0]);
									colorArray[1].SetFloat(color[1]);
									colorArray[2].SetFloat(color[2]);
									colorArray[3].SetFloat(color[3]);
									prefabModified = true;
								}
							}
						}
						}

					// === CAMERA COMPONENT ===
					else if (compData.type == "Camera") {
						if (compData.data.HasMember("position") && compData.data["position"].IsObject()) {
							auto& pos = compData.data["position"];
							float x = pos["x"].GetFloat();
							float y = pos["y"].GetFloat();

							if (ImGui::DragFloat2("Position", &x, 1.0f)) {
								pos["x"].SetFloat(x);
								pos["y"].SetFloat(y);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("size") && compData.data["size"].IsObject()) {
							auto& size = compData.data["size"];
							float width = size["width"].GetFloat();
							float height = size["height"].GetFloat();

							if (ImGui::DragFloat2("Size", &width, 1.0f, 1.0f, 10000.0f)) {
								size["width"].SetFloat(width);
								size["height"].SetFloat(height);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("zoom")) {
							float zoom = compData.data["zoom"].GetFloat();
							if (ImGui::DragFloat("Zoom", &zoom, 0.1f, 0.1f, 10.0f)) {
								compData.data["zoom"].SetFloat(zoom);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("target_entity")) {
							int targetEntity = compData.data["target_entity"].GetInt();
							if (ImGui::InputInt("Target Entity", &targetEntity)) {
								compData.data["target_entity"].SetInt(targetEntity);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("follow_speed")) {
							float followSpeed = compData.data["follow_speed"].GetFloat();
							if (ImGui::DragFloat("Follow Speed", &followSpeed, 0.1f, 0.0f, 100.0f)) {
								compData.data["follow_speed"].SetFloat(followSpeed);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("is_active")) {
							bool isActive = compData.data["is_active"].GetBool();
							if (ImGui::Checkbox("Is Active", &isActive)) {
								compData.data["is_active"].SetBool(isActive);
								prefabModified = true;
							}
						}
					}
						


					// === PLAYERSTATE COMPONENT ===
					else if (compData.type == "PlayerState") {
						if (compData.data.HasMember("is_facing_right")) {
							bool facingRight = compData.data["is_facing_right"].GetBool();
							if (ImGui::Checkbox("Is Facing Right", &facingRight)) {
								compData.data["is_facing_right"].SetBool(facingRight);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("is_grounded")) {
							bool isGrounded = compData.data["is_grounded"].GetBool();
							if (ImGui::Checkbox("Is Grounded", &isGrounded)) {
								compData.data["is_grounded"].SetBool(isGrounded);
								prefabModified = true;
							}
						}
					}

					// === BIRDAI COMPONENT ===
					else if (compData.type == "BirdAIComponent") {
						if (compData.data.HasMember("wakeup_radius")) {
							float wakeupRadius = compData.data["wakeup_radius"].GetFloat();
							if (ImGui::DragFloat("Wakeup Radius", &wakeupRadius, 0.1f, 0.0f, 100.0f)) {
								compData.data["wakeup_radius"].SetFloat(wakeupRadius);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("move_speed")) {
							float moveSpeed = compData.data["move_speed"].GetFloat();
							if (ImGui::DragFloat("Move Speed", &moveSpeed, 0.1f, 0.0f, 100.0f)) {
								compData.data["move_speed"].SetFloat(moveSpeed);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("attack_radius")) {
							float attackRadius = compData.data["attack_radius"].GetFloat();
							if (ImGui::DragFloat("Attack Radius", &attackRadius, 0.1f, 0.0f, 100.0f)) {
								compData.data["attack_radius"].SetFloat(attackRadius);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("attack_speed")) {
							float attackSpeed = compData.data["attack_speed"].GetFloat();
							if (ImGui::DragFloat("Attack Speed", &attackSpeed, 0.1f, 0.0f, 100.0f)) {
								compData.data["attack_speed"].SetFloat(attackSpeed);
								prefabModified = true;
							}
						}
					}

					// === SKELETONAI COMPONENT ===
					else if (compData.type == "SkeletonAIComponent") {
						if (compData.data.HasMember("detection_radius")) {
							float detectionRadius = compData.data["detection_radius"].GetFloat();
							if (ImGui::DragFloat("Detection Radius", &detectionRadius, 0.1f, 0.0f, 100.0f)) {
								compData.data["detection_radius"].SetFloat(detectionRadius);
								prefabModified = true;
							}
						}

						if (compData.data.HasMember("move_speed")) {
							float moveSpeed = compData.data["move_speed"].GetFloat();
							if (ImGui::DragFloat("Move Speed", &moveSpeed, 0.1f, 0.0f, 100.0f)) {
								compData.data["move_speed"].SetFloat(moveSpeed);
								prefabModified = true;
							}
						}
					}

					ImGui::Separator();
				}
			}

			// === ENTITY PROPERTIES SECTION ===
			ImGui::Separator();
			ImGui::SeparatorText("Entity Properties");

			// Collision Layer ID
			if (ImGui::InputInt("Collision Layer ID", &prefab->layer)) {
				prefabModified = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("The collision layer ID for physics interactions");
			}

			// Layer Collision Name
			static char layerCollisionNameBuffer[256] = "";
			strncpy_s(layerCollisionNameBuffer, sizeof(layerCollisionNameBuffer), prefab->layer_collision_name.c_str(), _TRUNCATE);
			if (ImGui::InputText("Collision Layer Name", layerCollisionNameBuffer, 256)) {
				prefab->layer_collision_name = std::string(layerCollisionNameBuffer);
				prefabModified = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("The collision layer name (e.g., 'Platform', 'Player', 'Default')");
			}

			ImGui::Spacing();

			// Render Layer
			if (ImGui::InputInt("Render Layer", &prefab->renderLayer)) {
				prefabModified = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("The rendering layer order (higher values render on top)");
			}

			// Custom Render Order
			if (ImGui::InputInt("Custom Render Order", &prefab->customRenderOrder)) {
				prefabModified = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Fine-tuned render order within the same render layer");
			}

			// Render Layer Name
			static char layerRenderNameBuffer[256] = "";
			strncpy_s(layerRenderNameBuffer, sizeof(layerRenderNameBuffer), prefab->layer_render_name.c_str(), _TRUNCATE);
			if (ImGui::InputText("Render Layer Name", layerRenderNameBuffer, 256)) {
				prefab->layer_render_name = std::string(layerRenderNameBuffer);
				prefabModified = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("The render layer name (optional)");
			}


			// === ADD/REMOVE COMPONENTS SECTION ===
			ImGui::Separator();
			ImGui::SeparatorText("Add/Remove Components");

			// Component dropdown for adding
			static int selectedAddComponentIndex = 0;
			static std::vector<std::string> availableComponents = {
				"None", "Velocity", "Acceleration", "PlayerState",
				"RotationFlag", "AudioComponent", "AnimationComponent",
				"TextComponent", "Camera", "ButtonComponent",
				"ParticleEmitterComponent", "BirdAIComponent",
				"SkeletonAIComponent", "NativeScripts", "TrapPlatformComponent"
			};

			std::vector<const char*> localComponentItems;
			for (const auto& comp : availableComponents) {
				localComponentItems.push_back(comp.c_str());
			}

			ImGui::Combo("Component to Add", &selectedAddComponentIndex, localComponentItems.data(), static_cast<int>(localComponentItems.size()));

			ImGui::SameLine();
			if (ImGui::Button("Add Component") && selectedAddComponentIndex > 0) {
				std::string compType = availableComponents[selectedAddComponentIndex];

				// Check if component already exists
				bool alreadyExists = false;
				for (const auto& comp : prefab->components) {
					if (comp.type == compType) {
						alreadyExists = true;
						break;
					}
				}

				if (!alreadyExists) {
					ComponentData newComp;
					newComp.type = compType;
					newComp.data.SetObject();
					rapidjson::Document::AllocatorType& alloc = newComp.data.GetAllocator();

					if (compType == "Velocity" || compType == "Acceleration") {
						newComp.data.AddMember("x", 0.0f, alloc);
						newComp.data.AddMember("y", 0.0f, alloc);
					}
					else if (compType == "PlayerState") {
						newComp.data.AddMember("is_facing_right", true, alloc);
						newComp.data.AddMember("is_grounded", false, alloc);
					}
					else if (compType == "RotationFlag") {
						newComp.data.AddMember("is_rotating", false, alloc);
						newComp.data.AddMember("current_rotation", 0.0f, alloc);
						newComp.data.AddMember("target_rotation", 0.0f, alloc);
					}
					else if (compType == "AudioComponent") {
						rapidjson::Value soundKey;
						soundKey.SetString("", alloc);
						newComp.data.AddMember("sound_key", soundKey, alloc);
						newComp.data.AddMember("should_loop", false, alloc);
						newComp.data.AddMember("is_streaming", false, alloc);
						newComp.data.AddMember("volume", 0.5f, alloc);
					}
					else if (compType == "AnimationComponent") {
						rapidjson::Value profileName;
						profileName.SetString("", alloc);
						newComp.data.AddMember("profile_name", profileName, alloc);
						newComp.data.AddMember("current_state", 0, alloc);
						newComp.data.AddMember("current_frame", 0, alloc);
						newComp.data.AddMember("frame_duration", 0.1f, alloc);
						newComp.data.AddMember("start_frame", 0, alloc);
						newComp.data.AddMember("frame_count", 1, alloc);
						newComp.data.AddMember("elapsed_time", 0.0f, alloc);
						newComp.data.AddMember("is_playing", true, alloc);
						newComp.data.AddMember("is_looping", true, alloc);
					}
					else if (compType == "TextComponent") {
						rapidjson::Value textVal, fontKeyVal;
						textVal.SetString("Text", alloc);
						fontKeyVal.SetString("", alloc);
						newComp.data.AddMember("text", textVal, alloc);
						newComp.data.AddMember("font_key", fontKeyVal, alloc);
						newComp.data.AddMember("font_size", 1.0f, alloc);
						newComp.data.AddMember("max_width", 100.0f, alloc);

						// Color
						rapidjson::Value colorArray(rapidjson::kArrayType);
						colorArray.PushBack(1.0f, alloc); // r
						colorArray.PushBack(1.0f, alloc); // g
						colorArray.PushBack(1.0f, alloc); // b
						colorArray.PushBack(1.0f, alloc); // a
						newComp.data.AddMember("font_color", colorArray, alloc);
					}
					else if (compType == "Camera") {
						rapidjson::Value posObj(rapidjson::kObjectType);
						posObj.AddMember("x", 0.0f, alloc);
						posObj.AddMember("y", 0.0f, alloc);
						newComp.data.AddMember("position", posObj, alloc);

						rapidjson::Value sizeObj(rapidjson::kObjectType);
						sizeObj.AddMember("width", 800.0f, alloc);
						sizeObj.AddMember("height", 600.0f, alloc);
						newComp.data.AddMember("size", sizeObj, alloc);

						newComp.data.AddMember("zoom", 1.0f, alloc);
						newComp.data.AddMember("target_entity", 0, alloc);
						newComp.data.AddMember("follow_speed", 5.0f, alloc);
						newComp.data.AddMember("is_active", true, alloc);
					}
					else if (compType == "ButtonComponent") {
						rapidjson::Value defaultTex, hoverTex, pressedTex;
						defaultTex.SetString("", alloc);
						hoverTex.SetString("", alloc);
						pressedTex.SetString("", alloc);

						newComp.data.AddMember("default_texture_key", defaultTex, alloc);
						newComp.data.AddMember("hover_texture_key", hoverTex, alloc);
						newComp.data.AddMember("pressed_texture_key", pressedTex, alloc);
						newComp.data.AddMember("default_is_animated", false, alloc);
						newComp.data.AddMember("hover_is_animated", false, alloc);
						newComp.data.AddMember("pressed_is_animated", false, alloc);

						newComp.data.AddMember("is_hovered", false, alloc);
						newComp.data.AddMember("is_pressed", false, alloc);
						newComp.data.AddMember("is_released", false, alloc);
						newComp.data.AddMember("action", 0, alloc);

						rapidjson::Value targetStr;
						targetStr.SetString("", alloc);
						newComp.data.AddMember("target_string", targetStr, alloc);
						newComp.data.AddMember("target_value", 0.0f, alloc);

						rapidjson::Value hoverSound, clickSound;
						hoverSound.SetString("", alloc);
						clickSound.SetString("", alloc);
						newComp.data.AddMember("hover_sound_key", hoverSound, alloc);
						newComp.data.AddMember("click_sound_key", clickSound, alloc);
						newComp.data.AddMember("hover_sound_volume", 0.5f, alloc);
						newComp.data.AddMember("click_sound_volume", 0.5f, alloc);
					}
					else if (compType == "ParticleEmitterComponent") {
						rapidjson::Value texKey;
						texKey.SetString("", alloc);
						newComp.data.AddMember("particle_texture_key", texKey, alloc);
						newComp.data.AddMember("is_emitting", true, alloc);
						newComp.data.AddMember("is_particle_animated", false, alloc);
						newComp.data.AddMember("is_screen_space", false, alloc);
						newComp.data.AddMember("particle_lifetime", 2.0f, alloc);
						newComp.data.AddMember("emit_rate", 10.0f, alloc);
						newComp.data.AddMember("emit_interval", 0.1f, alloc);
						newComp.data.AddMember("particle_effect_type", 0, alloc); // NONE
						newComp.data.AddMember("effect_strength", 1.0f, alloc);
						newComp.data.AddMember("particle_speed", 1.0f, alloc);
						newComp.data.AddMember("is_affected_by_gravity", false, alloc);
						newComp.data.AddMember("emission_style", 0, alloc); // FROM_POINT

						// Size
						rapidjson::Value sizeObj(rapidjson::kObjectType);
						sizeObj.AddMember("x", 10.0f, alloc);
						sizeObj.AddMember("y", 10.0f, alloc);
						newComp.data.AddMember("particle_size", sizeObj, alloc);
					}
					else if (compType == "BirdAIComponent") {
						newComp.data.AddMember("state", 0, alloc); // IDLE
						newComp.data.AddMember("is_facing_right", true, alloc);
						newComp.data.AddMember("state_timer", 0.0f, alloc);
						newComp.data.AddMember("wakeup_radius", 100.0f, alloc);
						newComp.data.AddMember("wakeup_time", 1.0f, alloc);
						newComp.data.AddMember("move_speed", 2.0f, alloc);
						newComp.data.AddMember("chase_radius", 200.0f, alloc);
						newComp.data.AddMember("hover_radius", 50.0f, alloc);
						newComp.data.AddMember("hover_height", 20.0f, alloc);
						newComp.data.AddMember("hover_speed", 1.0f, alloc);
						newComp.data.AddMember("attack_radius", 50.0f, alloc);
						newComp.data.AddMember("attack_speed", 5.0f, alloc);
						newComp.data.AddMember("attack_duration", 0.5f, alloc);
						newComp.data.AddMember("stun_duration", 1.0f, alloc);
						newComp.data.AddMember("path_recalculate_interval", 0.5f, alloc);
						newComp.data.AddMember("path_recalculate_timer", 0.0f, alloc);

						// Attack target position
						rapidjson::Value attackPos(rapidjson::kObjectType);
						attackPos.AddMember("x", 0.0f, alloc);
						attackPos.AddMember("y", 0.0f, alloc);
						newComp.data.AddMember("attack_target_position", attackPos, alloc);
					}
					else if (compType == "SkeletonAIComponent") {
						newComp.data.AddMember("state", 0, alloc); // IDLE
						newComp.data.AddMember("detection_radius", 100.0f, alloc);
						newComp.data.AddMember("move_speed", 2.0f, alloc);
					}
					else if (compType == "NativeScripts") {
						// Empty scripts array
						rapidjson::Value scriptsArray(rapidjson::kArrayType);
						newComp.data.AddMember("scripts", scriptsArray, alloc);
					}
					else if (compType == "TrapPlatformComponent") {
						newComp.data.AddMember("is_triggered", false, alloc);
						newComp.data.AddMember("has_fallen", false, alloc);
						newComp.data.AddMember("trigger_delay", 0.5f, alloc);
						newComp.data.AddMember("current_delay", 0.0f, alloc);
						newComp.data.AddMember("fall_speed", 100.0f, alloc);
						//newComp.data.AddMember("current_fall_distance", 0.0f, alloc);

						// Original position
						rapidjson::Value origPos(rapidjson::kObjectType);
						origPos.AddMember("x", 0.0f, alloc);
						origPos.AddMember("y", 0.0f, alloc);
						newComp.data.AddMember("original_position", origPos, alloc);
					}

					prefab->components.push_back(std::move(newComp));
					prefabModified = true;
					selectedAddComponentIndex = 0;
				}
			}

			// Remove component section
			ImGui::Spacing();
			ImGui::Text("Remove Component:");

			std::vector<std::string> removableComponents;
			removableComponents.push_back("None");
			for (const auto& comp : prefab->components) {
					removableComponents.push_back(comp.type);
			}

			static int selectedRemoveComponentIndex = 0;
			std::vector<const char*> removableItems;
			for (const auto& comp : removableComponents) {
				removableItems.push_back(comp.c_str());
			}

			ImGui::Combo("Component to Remove", &selectedRemoveComponentIndex, removableItems.data(), static_cast<int>(removableItems.size()));

			ImGui::SameLine();
			if (ImGui::Button("Remove Component") && selectedRemoveComponentIndex > 0) {
				std::string compToRemove = removableComponents[selectedRemoveComponentIndex];

				auto it = std::find_if(prefab->components.begin(), prefab->components.end(),
					[&compToRemove](const ComponentData& comp) {
						return comp.type == compToRemove;
					});

				if (it != prefab->components.end()) {
					prefab->components.erase(it);
					prefabModified = true;
					selectedRemoveComponentIndex = 0;
				}
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

				// TRANSFORM
				if (compData.type == "Transform") {
					if (entityMgr.HasComponent(entity, TransformType)) {
						Transform& transform = entityMgr.GetComponent<Transform>(entity);

						if (compData.data.HasMember("Rotation") && compData.data["Rotation"].IsObject()) {
							const auto& rot = compData.data["Rotation"];
							if (rot.HasMember("texture_angle"))
								transform.rotation.textureAngle = rot["texture_angle"].GetFloat();
							if (rot.HasMember("collider_angle"))
								transform.rotation.colliderAngle = rot["collider_angle"].GetFloat();

							if (transformationSystem_) {
								transformationSystem_->UpdateEntityTransform(entity);
							}
						}

						if (compData.data.HasMember("Scale") && compData.data["Scale"].IsObject()) {
							const auto& scale = compData.data["Scale"];
							if (scale.HasMember("x")) transform.scale.x = scale["x"].GetFloat();
							if (scale.HasMember("y")) transform.scale.y = scale["y"].GetFloat();
						}
					}
				}

				// RIGIDBODY
				else if (compData.type == "RigidBody") {
					if (entityMgr.HasComponent(entity, RigidBodyType)) {
						RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
						if (compData.data.HasMember("mass")) {
							rb.SetMass(compData.data["mass"].GetFloat());
						}
					}
				}

				// AABBCOLLIDER
				else if (compData.type == "AABBCollider") {
					if (entityMgr.HasComponent(entity, AABBColliderType)) {
						AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
						if (compData.data.HasMember("half_width") && compData.data.HasMember("half_height")) {
							col.HalfSize.x = compData.data["half_width"].GetFloat();
							col.HalfSize.y = compData.data["half_height"].GetFloat();
						}
						if (compData.data.HasMember("offset_x") && compData.data.HasMember("offset_y")) {
							col.Offset.x = compData.data["offset_x"].GetFloat();
							col.Offset.y = compData.data["offset_y"].GetFloat();
						}
					}
				}

				// TEXTURECOMPONENT
				else if (compData.type == "TextureComponent") {
					if (entityMgr.HasComponent(entity, TextureComponentType)) {
						TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);

						if (compData.data.HasMember("texture_key")) {
							tex.textureKey = compData.data["texture_key"].GetString();
						}

						if (compData.data.HasMember("half_size")) {
							const rapidjson::Value& halfSize = compData.data["half_size"];
							if (halfSize.IsObject() && halfSize.HasMember("x") && halfSize.HasMember("y")) {
								tex.HalfSize.x = halfSize["x"].GetFloat();
								tex.HalfSize.y = halfSize["y"].GetFloat();
							}
						}

						if (compData.data.HasMember("flip_x")) {
							tex.flipHorizontal = compData.data["flip_x"].GetBool();
						}
					}
				}

				// VELOCITY
				else if (compData.type == "Velocity") {
					if (entityMgr.HasComponent(entity, VelocityType)) {
						Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
						if (compData.data.HasMember("x")) vel.Vel.x = compData.data["x"].GetFloat();
						if (compData.data.HasMember("y")) vel.Vel.y = compData.data["y"].GetFloat();
					}
				}

				// ACCELERATION
				else if (compData.type == "Acceleration") {
					if (entityMgr.HasComponent(entity, AccelerationType)) {
						Acceleration& acc = entityMgr.GetComponent<Acceleration>(entity);
						if (compData.data.HasMember("x")) acc.Accel.x = compData.data["x"].GetFloat();
						if (compData.data.HasMember("y")) acc.Accel.y = compData.data["y"].GetFloat();
					}
				}

				// PLAYERSTATE
				else if (compData.type == "PlayerState") {
					if (entityMgr.HasComponent(entity, PlayerStateType)) {
						PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
						if (compData.data.HasMember("is_facing_right"))
							state.isFacingRight = compData.data["is_facing_right"].GetBool();
						if (compData.data.HasMember("is_grounded"))
							state.isGrounded = compData.data["is_grounded"].GetBool();
					}
				}

				// ROTATIONFLAG
				else if (compData.type == "RotationFlag") {
					if (entityMgr.HasComponent(entity, RotationFlagType)) {
						RotationFlag& rflag = entityMgr.GetComponent<RotationFlag>(entity);
						if (compData.data.HasMember("is_rotating"))
							rflag.isRotating = compData.data["is_rotating"].GetBool();
						if (compData.data.HasMember("target_rotation"))
							rflag.targetRotation = compData.data["target_rotation"].GetFloat();
					}
				}

				// AUDIOCOMPONENT
				else if (compData.type == "AudioComponent") {
					if (entityMgr.HasComponent(entity, AudioComponentType)) {
						AudioComponent& audio = entityMgr.GetComponent<AudioComponent>(entity);
						if (compData.data.HasMember("sound_key"))
							audio.soundKey = compData.data["sound_key"].GetString();
						if (compData.data.HasMember("should_loop"))
							audio.loop = compData.data["should_loop"].GetBool();
						if (compData.data.HasMember("is_streaming"))
							audio.stream = compData.data["is_streaming"].GetBool();
						if (compData.data.HasMember("volume"))
							audio.volume = compData.data["volume"].GetFloat();
					}
				}

				// ANIMATIONCOMPONENT
				else if (compData.type == "AnimationComponent") {
					if (entityMgr.HasComponent(entity, AnimationComponentType)) {
						AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);
						if (compData.data.HasMember("profile_name"))
							anim.profileName = compData.data["profile_name"].GetString();
						if (compData.data.HasMember("frame_count"))
							anim.frameCount = compData.data["frame_count"].GetInt();
						if (compData.data.HasMember("frame_duration"))
							anim.frameDuration = compData.data["frame_duration"].GetFloat();
						if (compData.data.HasMember("start_frame"))
							anim.startFrame = compData.data["start_frame"].GetInt();
						if (compData.data.HasMember("is_looping"))
							anim.isLooping = compData.data["is_looping"].GetBool();
					}
				}

				// BIRDAI COMPONENT
			/*	else if (compData.type == "BirdAIComponent") {
					if (entityMgr.HasComponent(entity, BirdAIComponentType)) {
						BirdAIComponent& birdAI = entityMgr.GetComponent<BirdAIComponent>(entity);
						if (compData.data.HasMember("wakeup_radius"))
							birdAI.wakeUpRadius = compData.data["wakeup_radius"].GetFloat();
						if (compData.data.HasMember("move_speed"))
							birdAI.moveSpeed = compData.data["move_speed"].GetFloat();
						if (compData.data.HasMember("attack_radius"))
							birdAI.attackRadius = compData.data["attack_radius"].GetFloat();
						if (compData.data.HasMember("attack_speed"))
							birdAI.attackSpeed = compData.data["attack_speed"].GetFloat();
						if (compData.data.HasMember("wakeup_time"))
							birdAI.wakeUpTime = compData.data["wakeup_time"].GetFloat();
						if (compData.data.HasMember("hover_radius"))
							birdAI.hoverRadius = compData.data["hover_radius"].GetFloat();
						if (compData.data.HasMember("hover_height"))
							birdAI.hoverHeight = compData.data["hover_height"].GetFloat();
						if (compData.data.HasMember("hover_speed"))
							birdAI.hoverSpeed = compData.data["hover_speed"].GetFloat();
					}
				}*/

				//// SKELETONAI COMPONENT
				//else if (compData.type == "SkeletonAIComponent") {
				//	if (entityMgr.HasComponent(entity, SkeletonAIComponentType)) {
				//		SkeletonAIComponent& skelAI = entityMgr.GetComponent<SkeletonAIComponent>(entity);
				//		if (compData.data.HasMember("detection_radius"))
				//			skelAI.detectionRadius = compData.data["detection_radius"].GetFloat();
				//		if (compData.data.HasMember("move_speed"))
				//			skelAI.moveSpeed = compData.data["move_speed"].GetFloat();
				//	}
				//}

				// TRAPPED PLATFORM COMPONENT
				else if (compData.type == "TrapPlatformComponent") {
					if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
						TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
						if (compData.data.HasMember("fall_speed"))
							trap.fallSpeed = compData.data["fall_speed"].GetFloat();
						if (compData.data.HasMember("trigger_delay"))
							trap.triggerDelay = compData.data["trigger_delay"].GetFloat();
					}
				}
				else if (compData.type == "TextComponent") {
					if (entityMgr.HasComponent(entity, TextComponentType)) {
						TextComponent& text = entityMgr.GetComponent<TextComponent>(entity);
						if (compData.data.HasMember("text"))
							text.textToDraw = compData.data["text"].GetString();
						if (compData.data.HasMember("font_key"))
							text.fontKey = compData.data["font_key"].GetString();
						if (compData.data.HasMember("font_size"))
							text.scale = compData.data["font_size"].GetFloat();
						if (compData.data.HasMember("max_width"))
							text.maxWidth = compData.data["max_width"].GetFloat();

						// Update color
						if (compData.data.HasMember("font_color") && compData.data["font_color"].IsArray()) {
							const auto& colorArray = compData.data["font_color"];
							if (colorArray.Size() >= 4) {
								text.color.r = colorArray[0].GetFloat();
								text.color.g = colorArray[1].GetFloat();
								text.color.b = colorArray[2].GetFloat();
								text.color.a = colorArray[3].GetFloat();
							}
						}
						text.isDirty = true;
					}
				}

				// === CAMERA COMPONENT ===
				else if (compData.type == "Camera") {
					if (entityMgr.HasComponent(entity, CameraType)) {
						Camera& camera = entityMgr.GetComponent<Camera>(entity);
						if (compData.data.HasMember("position") && compData.data["position"].IsObject()) {
							const auto& pos = compData.data["position"];
							if (pos.HasMember("x")) camera.position.x = pos["x"].GetFloat();
							if (pos.HasMember("y")) camera.position.y = pos["y"].GetFloat();
						}
						if (compData.data.HasMember("size") && compData.data["size"].IsObject()) {
							const auto& size = compData.data["size"];
							if (size.HasMember("width")) camera.size.x = size["width"].GetFloat();
							if (size.HasMember("height")) camera.size.y = size["height"].GetFloat();
						}
						if (compData.data.HasMember("zoom"))
							camera.zoom = compData.data["zoom"].GetFloat();
						if (compData.data.HasMember("target_entity"))
							camera.targetEntity = compData.data["target_entity"].GetInt();
						if (compData.data.HasMember("follow_speed"))
							camera.followSpeed = compData.data["follow_speed"].GetFloat();
						if (compData.data.HasMember("is_active"))
							camera.isActive = compData.data["is_active"].GetBool();
					}
				}
						

				// === BUTTON COMPONENT ===
				else if (compData.type == "ButtonComponent") {
					if (entityMgr.HasComponent(entity, ButtonType)) {
						Button& button = entityMgr.GetComponent<Button>(entity);
						if (compData.data.HasMember("default_texture_key"))
							button.defaultTextureKey = compData.data["default_texture_key"].GetString();
						if (compData.data.HasMember("hover_texture_key"))
							button.hoverTextureKey = compData.data["hover_texture_key"].GetString();
						if (compData.data.HasMember("pressed_texture_key"))
							button.pressedTextureKey = compData.data["pressed_texture_key"].GetString();
						if (compData.data.HasMember("default_is_animated"))
							button.defaultIsAnimated = compData.data["default_is_animated"].GetBool();
						if (compData.data.HasMember("hover_is_animated"))
							button.hoverIsAnimated = compData.data["hover_is_animated"].GetBool();
						if (compData.data.HasMember("pressed_is_animated"))
							button.pressedIsAnimated = compData.data["pressed_is_animated"].GetBool();
						if (compData.data.HasMember("action"))
							button.action = static_cast<ButtonAction>(compData.data["action"].GetInt());
						if (compData.data.HasMember("target_string"))
							button.targetString = compData.data["target_string"].GetString();
						if (compData.data.HasMember("target_value"))
							button.targetValue = compData.data["target_value"].GetFloat();
					}
				}
							
			}
		}

		Logger::Get().Info(DebugType::Editor)
			<< "Applied prefab changes for " << prefabName << " to " << entities.size() << " entities" << std::endl;
	}


	// Save prefab changes to JSON file
	void Editor::SavePrefabToFile(const std::string& prefabName) {
		using namespace Framework;

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

		//// Get the AI system from game manager
		//AISystem* aiSystem = gameManager_->GetSystems().aiSystem_;

		//if (!aiSystem) {
		//	ImGui::TextDisabled("AI System not available");
		//	ImGui::End();
		//	return;
		//}

		// Master toggle
		//ImGui::Checkbox("Enable AI Debug Visualization", &aiSystem->debugVisualization);
		//ImGui::Separator();

		//// Individual toggles
		//ImGui::BeginDisabled(!aiSystem->debugVisualization);

		//ImGui::Text("Visualization Options:");
		//ImGui::Checkbox("Show Navigation Grid", &aiSystem->showNavigationGrid);
		//ImGui::Checkbox("Show Bird Debug Info", &aiSystem->showBirdDebug);
		///*ImGui::Checkbox("Show Paths", &aiSystem->showPaths);
		//ImGui::Checkbox("Show Colliders", &aiSystem->showColliders);*/

		//ImGui::EndDisabled();

		//ImGui::Separator();

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

		//// Color legend
		//if (ImGui::CollapsingHeader("State Colors Legend")) {
		//	ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Gray = IDLE");
		//	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow = WAKING");
		//	ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Orange = CHASE");
		//	ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Cyan = HOVER");
		//	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red = ATTACK");
		//}

		//if (ImGui::CollapsingHeader("Radius Legend")) {
		//	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "Yellow Circle = Wake-up Radius");
		//	ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.7f, 1.0f), "Cyan Circle = Hover Radius");
		//	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red Circle = Attack Radius");
		//}

		//if (ImGui::CollapsingHeader("Grid Legend")) {
		//	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Green Cells = Walkable");
		//	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red Cells = Blocked");
		//}

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
			if (entityMgr.HasComponent(entity, Framework::ComponentType::TransformType))
			{
				bool isGUI = entityMgr.HasComponent(entity, GUITagType);
				Vector2D checkPos = isGUI ? GetNormalizedMousePos() : mouseWorldPos;

				Vector2D halfSize{};
				if (entityMgr.HasComponent(entity, Framework::ComponentType::TextComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextComponent>(entity).halfSize;
				}
				else if (entityMgr.HasComponent(entity, Framework::ComponentType::TextureComponentType)) {
					halfSize = entityMgr.GetComponent<Framework::TextureComponent>(entity).HalfSize;
				}
				else if (entityMgr.HasComponent(entity, Framework::ComponentType::AABBColliderType)) {
					halfSize = entityMgr.GetComponent<Framework::AABBCollider>(entity).HalfSize;
				}
				else {
					continue;
				}

				Transform transformComp = entityMgr.GetComponent<Framework::Transform>(entity);
				Vector2D& pos = transformComp.position;
				Vector2D& scale = transformComp.scale;

				// Apply Scale to HalfSize
				Vector2D scaledHalfSize = { halfSize.x * scale.x, halfSize.y * scale.y };

				//  Check bounds using scaledHalfSize
				if (checkPos.x >= pos.x - scaledHalfSize.x && checkPos.x <= pos.x + scaledHalfSize.x &&
					checkPos.y >= pos.y - scaledHalfSize.y && checkPos.y <= pos.y + scaledHalfSize.y)
				{
					bestCandidate = entity;
					break;
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

#pragma region COMPONENT INSPECTOR

	void Editor::DrawTransformInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		Transform& transform = entityMgr.GetComponent<Transform>(entity);
		ComponentType compType = ComponentType::TransformType;

		bool nowActive = false;

		// --- Position ---
		ImGui::InputFloat2("Position", &transform.position.x, "%.2f");
		if (ImGui::IsItemActive()) nowActive = true;

		// --- Rotation ---
		static bool unifiedTransformRotation = true;
		ImGui::Checkbox("Rotation (Texture + Collider)", &unifiedTransformRotation);
		ImGui::Separator();

		if (unifiedTransformRotation) {
			float unifiedDegrees = static_cast<float>(transform.rotation.textureAngle * (180.0f / M_PI));

			ImGui::SetNextItemWidth(100.f);
			if (ImGui::DragFloat("Rotation (Degrees)", &unifiedDegrees, 1.0f, -180.0f, 180.0f, "%.2f")) {
				float newRadians = static_cast<float>(unifiedDegrees * (M_PI / 180.0f));
				transform.rotation.textureAngle = newRadians;
				transform.rotation.colliderAngle = newRadians;
				if (transformationSystem_) {
					transformationSystem_->UpdateEntityTransform(entity);
				}
				if (gameManager_->GetSystems().aiSystem_) gameManager_->GetSystems().aiSystem_->ForceGridRebuild();
			}
			if (ImGui::IsItemActive()) nowActive = true;

			if (ImGui::Button("Reset Rotation")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				transform.rotation.textureAngle = 0.0f;
				transform.rotation.colliderAngle = 0.0f;
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);

				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) {
					std::stringstream ss;
					ss << "Reset Rotation in " << entityMgr.GetProperties(entity).name << std::endl;
					historyManager_.RecordModifyComponent(entity, ss.str(), beforeJSON, afterJSON, compType);
				}
			}
			ImGui::Text("Current: %.1f°", unifiedDegrees);
		}
		else {
			// Texture Rotation
			ImGui::Text("Texture Rotation");
			float textureDegrees = static_cast<float>(transform.rotation.textureAngle * (180.0f / M_PI));
			if (ImGui::DragFloat("Texture (Degrees)", &textureDegrees, 1.0f, -180.0f, 180.0f, "%.2f")) {
				transform.rotation.textureAngle = static_cast<float>(textureDegrees * (M_PI / 180.0f));
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);
			}
			if (ImGui::IsItemActive()) nowActive = true;

			ImGui::Separator();

			// Collider Rotation
			ImGui::Text("Collider Rotation");
			float colliderDegrees = static_cast<float>(transform.rotation.colliderAngle * (180.0f / M_PI));
			if (ImGui::DragFloat("Collider (Degrees)", &colliderDegrees, 1.0f, -180.0f, 180.0f, "%.2f")) {
				transform.rotation.colliderAngle = static_cast<float>(colliderDegrees * (M_PI / 180.0f));
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);
			}
			if (ImGui::IsItemActive()) nowActive = true;

			ImGui::Separator();

			// Reset Buttons
			if (ImGui::Button("Reset Texture")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				transform.rotation.textureAngle = 0.0f;
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Reset Texture Rotation", beforeJSON, afterJSON, compType);
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Collider")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				transform.rotation.colliderAngle = 0.0f;
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Reset Collider Rotation", beforeJSON, afterJSON, compType);
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Both")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				transform.rotation.textureAngle = 0.0f;
				transform.rotation.colliderAngle = 0.0f;
				if (transformationSystem_) transformationSystem_->UpdateEntityTransform(entity);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Reset Rotation", beforeJSON, afterJSON, compType);
			}
			ImGui::Text("Texture: %.1f° | Collider: %.1f°", textureDegrees, colliderDegrees);
		}

		// --- Scaling ---
		if (transformationSystem_) {
			static std::unordered_map<Entity, Vector2D> defaultSizes;

			// Cache the default collider size if the entity has one
			bool hasCollider = entityMgr.HasComponent(entity, AABBColliderType);
			if (hasCollider && defaultSizes.find(entity) == defaultSizes.end()) {
				defaultSizes[entity] = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
			}

			// Always expose scale controls for the transform
			Vector2D ScaleFactor = { transform.scale.x, transform.scale.y };

			// --- Non-Uniform Scaling ---
			if (ImGui::DragFloat2("Scale (X, Y)", &ScaleFactor.x, 0.1f, 0.1f, 5.0f, "%.2f")) {
				transform.scale.x = ScaleFactor.x;
				transform.scale.y = ScaleFactor.y;

				// Only scale the collider if it exists
				if (hasCollider && defaultSizes.count(entity)) {
					AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
					collider.HalfSize.x = defaultSizes[entity].x * ScaleFactor.x;
					collider.HalfSize.y = defaultSizes[entity].y * ScaleFactor.y;
				}

				transformationSystem_->UpdateEntityTransform(entity);
				if (gameManager_->GetSystems().aiSystem_) gameManager_->GetSystems().aiSystem_->ForceGridRebuild();
			}
			if (ImGui::IsItemActive()) nowActive = true;

			// --- Uniform Scaling ---
			static bool uniformTransformScale = false;
			ImGui::Checkbox("Uniform Scaling", &uniformTransformScale);

			if (uniformTransformScale) {
				float uniformValue = (ScaleFactor.x + ScaleFactor.y) * 0.5f;
				if (ImGui::DragFloat("Uniform Scale", &uniformValue, 0.1f, 0.1f, 5.0f, "%.2f")) {
					transform.scale.x = uniformValue;
					transform.scale.y = uniformValue;

					if (hasCollider && defaultSizes.count(entity)) {
						AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
						collider.HalfSize.x = defaultSizes[entity].x * uniformValue;
						collider.HalfSize.y = defaultSizes[entity].y * uniformValue;
					}

					transformationSystem_->UpdateEntityTransform(entity);
					if (gameManager_->GetSystems().aiSystem_) gameManager_->GetSystems().aiSystem_->ForceGridRebuild();
				}
				if (ImGui::IsItemActive()) nowActive = true;
			}

			// --- Reset Scale ---
			if (ImGui::Button("Reset Scale")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

				transform.scale = { 1.0f, 1.0f };

				if (hasCollider && defaultSizes.count(entity)) {
					AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
					collider.HalfSize = defaultSizes[entity];
				}

				transformationSystem_->UpdateEntityTransform(entity);

				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Reset Scale", beforeJSON, afterJSON, compType);
			}

			// Show a helpful indicator if they are scaling just a texture
			if (!hasCollider) {
				ImGui::TextDisabled("No AABBCollider attached (Scaling Texture only)");
			}
		}
		else {
			Framework::Logger::Get().Error(DebugType::Editor) << "Transformation system not initialised" << std::endl;
		}

		// History Tracking logic
		// Only track edits if this is the entity currently being edited (or no edit is active)
		if (!editSession.active || editSession.entityInEdit == entity) {
			if (nowActive && !editSession.wasItemActive) {
				editSession.Begin(entity, compType);
			}
			if (!nowActive && editSession.wasItemActive) {
				std::stringstream ss;
				ss << "Set Transform in " << entityMgr.GetProperties(entity).name << std::endl;
				editSession.End(entity, compType, ss.str(), historyManager_);
			}
			editSession.wasItemActive = nowActive;
		}
	}

	void Editor::DrawRigidBodyInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
		ComponentType compType = ComponentType::RigidBodyType;

		// Force - Read Only
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::InputFloat2("Force", &rb.Force.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(2);

		// Mass
		float localMass = rb.Mass;
		if (ImGui::InputFloat("Mass", &localMass, 0.1f, 1.0f, "%.2f")) {
			rb.SetMass(localMass);
		}

		bool activeNow = ImGui::IsItemActive();
		if (activeNow && !editSession.wasItemActive) {
			editSession.Begin(entity, compType);
		}

		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Mass in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}

		// Inverse Mass - Read Only
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::InputFloat("Inverse Mass", &rb.InverseMass, 0.f, 0.f, "%.2f", ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(2);

		editSession.wasItemActive = activeNow;
	}

	void Editor::DrawTextureInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);
		ComponentType compType = ComponentType::TextureComponentType;

		// Read-only Texture Key display
		ImGui::InputText("Texture Key", &tex.textureKey, ImGuiInputTextFlags_ReadOnly);

		// Drag and Drop Target for Texture Key
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
			if (payload)
			{
				const char* dropped_path_c_str = (const char*)payload->Data;
				fs::path dropped_path(dropped_path_c_str);
				std::string ext = dropped_path.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
				{
					std::string newKey = dropped_path.filename().string();
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
					tex.textureKey = newKey;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

					if (beforeJSON != afterJSON) {
						historyManager_.RecordModifyComponent(entity, "Modified Texture Key (DragDrop)", beforeJSON, afterJSON, compType);
					}

					resourceManager_->unload<Texture>(newKey);
					resourceManager_->loadTexture(newKey, dropped_path.string());
					Logger::Get().Info(DebugType::Editor) << "Texture key updated to: " << newKey << std::endl;
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Manual input field
		static std::string newTextureKey = "";
		bool activeNow = false;

		ImGui::InputText("New Texture Key", &newTextureKey);

		if (ImGui::Button("Update Texture")) {
			if (!newTextureKey.empty()) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				tex.textureKey = newTextureKey;
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

				if (beforeJSON != afterJSON) {
					historyManager_.RecordModifyComponent(entity, "Modified Texture Key (Manual)", beforeJSON, afterJSON, compType);
				}
			}
		}

		if (ImGui::InputFloat2("Half Size", &tex.HalfSize.x, "%.2f")) {
			activeNow = true;
			if (gameManager_->GetSystems().aiSystem_) gameManager_->GetSystems().aiSystem_->ForceGridRebuild();
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::Checkbox("Flip Horizontal", &tex.flipHorizontal)) activeNow = true;
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::SliderFloat("Opacity", &tex.alpha, 0.0f, 1.0f, "%.2f")) activeNow = true;
		if (ImGui::IsItemActive()) activeNow = true;

		// UV Override Section
		ImGui::Separator();
		ImGui::SeparatorText("UV Override");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("When enabled, the renderer uses these UV coordinates directly\ninstead of the AnimationSystem (used for slider fills, static crops, etc.)");
		}

		if (ImGui::Checkbox("Override UV", &tex.overrideUV)) activeNow = true;
		if (ImGui::IsItemActive()) activeNow = true;

		if (tex.overrideUV) {
			ImGui::BeginDisabled(!tex.overrideUV);
			if (ImGui::InputFloat2("Min UV (U0, V0)", &tex.minUV.x, "%.3f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;

			if (ImGui::InputFloat2("Max UV (U1, V1)", &tex.maxUV.x, "%.3f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;

			if (ImGui::Button("Reset UV")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				tex.minUV = { 0.0f, 0.0f };
				tex.maxUV = { 1.0f, 1.0f };
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON) {
					historyManager_.RecordModifyComponent(entity, "Reset UV Override", beforeJSON, afterJSON, compType);
				}
			}
			ImGui::EndDisabled();
		}

		if (activeNow && !editSession.wasItemActive) {
			editSession.Begin(entity, compType);
		}
		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Texture Properties in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}
		editSession.wasItemActive = activeNow;
	}

	void Editor::DrawAnimationInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
		ComponentType compType = ComponentType::AnimationComponentType;

		bool activeNow = false;

		// Show the profile name for AnimationComponent
		if (ImGui::InputText("Animation Profile", &animComp.profileName)) {
			// Trigger a refresh/reload if necessary in the future
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::InputInt("Frame Count", &animComp.frameCount)) {
			if (animComp.frameCount < 1) animComp.frameCount = 1;
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::InputFloat("Frame Duration", &animComp.frameDuration)) {
			if (animComp.frameDuration < 0.001f) animComp.frameDuration = 0.001f;
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::Checkbox("Is Playing", &animComp.isPlaying)) {
			// If we unpause, we might want to reset elapsed time, but for now just toggle
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::Checkbox("Is Looping", &animComp.isLooping)) activeNow = true;
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::InputInt("Start Frame", &animComp.startFrame)) {
			if (animComp.startFrame < 0) animComp.startFrame = 0;
			if (animComp.startFrame >= animComp.frameCount) animComp.startFrame = animComp.frameCount - 1;
		}
		if (ImGui::IsItemActive()) activeNow = true;

		ImGui::Separator();
		ImGui::Text("Start Delay");
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delays the animation before it begins playing");

		if (ImGui::InputFloat("Delay (seconds)", &animComp.startDelay)) {
			if (animComp.startDelay < 0.0f) animComp.startDelay = 0.0f;
		}
		if (ImGui::IsItemActive()) activeNow = true;

		if (ImGui::InputFloat("Delay Range (seconds)", &animComp.startDelayRange)) {
			if (animComp.startDelayRange < 0.0f) animComp.startDelayRange = 0.0f;
		}
		if (ImGui::IsItemActive()) activeNow = true;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds a random extra delay in [0, range] on top of the base delay");

		// History Tracking
		if (activeNow && !editSession.wasItemActive) {
			editSession.Begin(entity, compType);
		}
		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Animation Properties in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}
		editSession.wasItemActive = activeNow;
	}

	void Editor::DrawAudioInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(entity);
		ComponentType compType = ComponentType::AudioComponentType;

		ImGui::InputText("Audio Key", &audioComp.soundKey, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
			if (payload)
			{
				std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
				fs::path dropped_file(dropped_path);
				std::string ext = dropped_file.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

				if (ext == ".ogg" || ext == ".wav" || ext == ".mp3")
				{
					std::string newKey = dropped_file.filename().string();
					std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
					audioComp.soundKey = newKey;
					std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

					if (beforeJSON != afterJSON) {
						historyManager_.RecordModifyComponent(entity, "Modified Audio Key (DragDrop)", beforeJSON, afterJSON, compType);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		static std::string newSoundKey = "";
		ImGui::InputText("New Audio Key", &newSoundKey);

		if (ImGui::Button("Update Audio")) {
			if (!newSoundKey.empty()) {
				fs::path filePath(newSoundKey);
				if (filePath.has_extension()) {
					std::string ext = filePath.extension().string();
					if (ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
						std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
						audioComp.soundKey = newSoundKey;
						std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
						if (beforeJSON != afterJSON) {
							historyManager_.RecordModifyComponent(entity, "Modified Audio Key", beforeJSON, afterJSON, compType);
						}
					}
				}
			}
		}

		const char* audioTypeItems[] = { "SFX", "Music" };
		int currentAudioTypeIndex = static_cast<int>(audioComp.audioType);

		if (ImGui::Combo("Audio Type", &currentAudioTypeIndex, audioTypeItems, IM_ARRAYSIZE(audioTypeItems))) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			audioComp.audioType = static_cast<AudioType>(currentAudioTypeIndex);
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Modified Audio Type", beforeJSON, afterJSON, compType);
		}

		bool loopLocal = audioComp.loop;
		if (ImGui::Checkbox("Looping", &loopLocal)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			audioComp.loop = loopLocal;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Modified Audio Loop", beforeJSON, afterJSON, compType);
		}

		bool streamLocal = audioComp.stream;
		if (ImGui::Checkbox("Stream", &streamLocal)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			audioComp.stream = streamLocal;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Modified Audio Stream", beforeJSON, afterJSON, compType);
		}

		ImGui::SliderFloat("Volume", &audioComp.volume, 0.0f, 1.0f, "%.2f");
		bool activeNow = ImGui::IsItemActive();

		if (activeNow && !editSession.wasItemActive) {
			editSession.Begin(entity, compType);
		}
		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Audio Volume in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}
		editSession.wasItemActive = activeNow;
	}

	void Editor::DrawButtonInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		Button& buttonComp = entityMgr.GetComponent<Button>(entity);
		ComponentType compType = ComponentType::ButtonType;

		bool activeNow = false;

		bool isPressedLocal = buttonComp.isPressed;
		if (ImGui::Checkbox("Is Pressed?", &isPressedLocal)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			buttonComp.isPressed = isPressedLocal;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

			if (beforeJSON != afterJSON) {
				historyManager_.RecordModifyComponent(entity, "Modified Button Pressed State", beforeJSON, afterJSON, compType);
			}
		}

		ImGui::Separator();
		ImGui::SeparatorText("Button States");

		bool stateChanged = false;
		std::string stateBeforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

		// Added optional parameters for sound key and volume
		auto HandleStateUI = [&](const char* label, std::string& texKey, bool& isAnim, std::string* soundKey = nullptr, float* soundVol = nullptr) {
			ImGui::PushID(label);

			if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {

				// 1. Texture Properties
				ImGui::Text("Texture Key:");
				ImGui::InputText("##Texture", &texKey);
				if (ImGui::IsItemActive()) activeNow = true;

				if (ImGui::BeginDragDropTarget()) {
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
					if (payload) {
						std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
						fs::path dropped_file(dropped_path);

						texKey = dropped_file.filename().string();
						stateChanged = true;

						resourceManager_->unload<Texture>(texKey);
						resourceManager_->loadTexture(texKey, dropped_file.string());
					}
					ImGui::EndDragDropTarget();
				}
				if (ImGui::Checkbox("Is Animated", &isAnim)) stateChanged = true;

				// 2. Sound Properties (Only renders if sound variables are passed in)
				if (soundKey && soundVol) {
					ImGui::Spacing();
					ImGui::Text("Sound Key:");
					ImGui::InputText("##Sound", soundKey);
					if (ImGui::IsItemActive()) activeNow = true;

					if (ImGui::BeginDragDropTarget()) {
						const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
						if (payload) {
							std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
							fs::path dropped_file(dropped_path);
							std::string ext = dropped_file.extension().string();
							std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

							if (ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
								*soundKey = dropped_file.filename().string();
								stateChanged = true;
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::Text("Volume:");
					ImGui::SliderFloat("##Volume", soundVol, 0.0f, 1.0f, "%.2f");
					if (ImGui::IsItemActive()) activeNow = true;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			ImGui::Spacing();
			};

		// Pass the specific properties for each state. Default gets no sound pointers.
		HandleStateUI("Default State", buttonComp.defaultTextureKey, buttonComp.defaultIsAnimated);
		HandleStateUI("Hover State", buttonComp.hoverTextureKey, buttonComp.hoverIsAnimated, &buttonComp.hoverSoundKey, &buttonComp.hoverSoundVolume);
		HandleStateUI("Pressed State", buttonComp.pressedTextureKey, buttonComp.pressedIsAnimated, &buttonComp.clickSoundKey, &buttonComp.clickSoundVolume);

		if (stateChanged) {
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (stateBeforeJSON != afterJSON) {
				historyManager_.RecordModifyComponent(entity, "Modified Button States", stateBeforeJSON, afterJSON, compType);
			}
		}

		ImGui::Separator();
		ImGui::SeparatorText("Button Actions");

		const char* items[] = { "DEFAULT", "PLAYSOUND", "RESETPLAYERPOS", "SWITCH_SCENE", "SWITCH_SCENE_ADD", "QUIT_GAME", "PREV_PAGE", "NEXT_PAGE", "RESUME_GAME"};
		int currentItemIndex = static_cast<int>(buttonComp.action);

		if (ImGui::Combo("Button Action", &currentItemIndex, items, IM_ARRAYSIZE(items))) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			buttonComp.action = static_cast<ButtonAction>(currentItemIndex);
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

			if (beforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, "Modified Button Action", beforeJSON, afterJSON, compType);
		}

		bool dataChanged = false;
		std::string dataBeforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);

		if (buttonComp.action == ButtonAction::SWITCH_SCENE || buttonComp.action == ButtonAction::SWITCH_SCENE_ADD) {
			ImGui::Text("Target Scene Name:");
			ImGui::InputText("##SceneName", &buttonComp.targetString);
			if (ImGui::IsItemActive()) activeNow = true;

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
					std::string dropped_path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
					fs::path dropped_file(dropped_path);
					buttonComp.targetString = dropped_file.stem().string();
					dataChanged = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::Text("Scene to Unload on Click:");
			ImGui::InputText("##SceneToUnload", &buttonComp.sceneToUnloadOnClick);
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Optional: Name of the scene to unload when this button is clicked.\nLeave empty to skip unloading.");
			}
		}
		else if (buttonComp.action == ButtonAction::PLAYSOUND) {
			ImGui::Text("Sound Key:");
			ImGui::InputText("##SoundKey", &buttonComp.targetString);
			if (ImGui::IsItemActive()) activeNow = true;
		}
		else if (buttonComp.action == ButtonAction::DEFAULT) {
			ImGui::Text("Event Value:");
			ImGui::DragFloat("##Val", &buttonComp.targetValue);
			if (ImGui::IsItemActive()) activeNow = true;
		}

		if (dataChanged) {
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			std::stringstream ss;
			ss << "Modified Button Data in " << entityMgr.GetProperties(entity).name << std::endl;
			if (dataBeforeJSON != afterJSON) historyManager_.RecordModifyComponent(entity, ss.str(), dataBeforeJSON, afterJSON, compType);
		}

		if (activeNow && !editSession.wasItemActive) {
			editSession.Begin(entity, compType);
		}

		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Button Data in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}

		editSession.wasItemActive = activeNow;
	}

	void Editor::DrawCameraInspector(Entity entity) {
		using namespace Framework;
		EntityStorage* ecs = gameManager_->GetSystems().ecs_;
		EntityManager entityMgr(*ecs);
		Camera& cam = entityMgr.GetComponent<Camera>(entity);
		ComponentType compType = ComponentType::CameraType;

		bool activeNow = false;

		// --- Is Active ---
		bool isActiveCopy = cam.isActive;
		if (ImGui::Checkbox("Is Active", &isActiveCopy)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			cam.isActive = isActiveCopy;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON)
				historyManager_.RecordModifyComponent(entity, "Modified Camera IsActive", beforeJSON, afterJSON, compType);
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("When enabled, this is the camera used for rendering during gameplay.");

		// --- Is Editor Controlled ---
		bool editorCtrlCopy = cam.isEditorControlled;
		if (ImGui::Checkbox("Editor Controlled", &editorCtrlCopy)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			cam.isEditorControlled = editorCtrlCopy;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON)
				historyManager_.RecordModifyComponent(entity, "Modified Camera EditorControlled", beforeJSON, afterJSON, compType);
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("When enabled, the game cannot move this camera (reserved for the level editor camera).");

		ImGui::Separator();

		// --- Position (read-only, driven by system) ---
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::InputFloat2("Position", &cam.position.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(2);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("World-space position the camera is looking at. Updated at runtime by the follow system.");

		// --- Size (read-only, driven by zoom) ---
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::InputFloat2("Size (W, H)", &cam.size.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(2);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Visible world-space area (width x height). Controlled by Zoom.");

		// --- Zoom ---
		float zoomCopy = cam.zoom;
		if (ImGui::DragFloat("Zoom", &zoomCopy, 1.0f, 1.0f, 10000.0f, "%.1f")) {
			if (cameraSystem_) cameraSystem_->SetZoom(entity, zoomCopy);
			activeNow = true;
		}
		if (ImGui::IsItemActive()) activeNow = true;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Higher values zoom out (more world visible). Lower values zoom in.");

		// --- Aspect Ratio (read-only, driven by viewport) ---
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::InputFloat("Aspect Ratio", &cam.aspectRatio, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_ReadOnly);
		ImGui::PopStyleColor(2);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Width / Height ratio, set automatically from the viewport.");

		ImGui::Separator();
		ImGui::SeparatorText("Follow Target");

		// --- Target Entity ---
		int targetInt = static_cast<int>(cam.targetEntity);
		if (ImGui::InputInt("Target Entity ID", &targetInt)) {
			if (targetInt < 0) targetInt = 0;
			activeNow = true;
		}
		if (ImGui::IsItemActive()) activeNow = true;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Entity ID to follow. Set to 0 to disable camera following.");

		// --- Follow Speed ---
		float followSpeedCopy = cam.followSpeed;
		if (ImGui::DragFloat("Follow Speed", &followSpeedCopy, 0.1f, 0.0f, 100.0f, "%.2f")) {
			activeNow = true;
		}
		if (ImGui::IsItemActive()) activeNow = true;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Interpolation speed toward the target entity. 0 = instant snap.");

		// Apply follow target changes together
		if (!activeNow && editSession.wasItemActive) {
			cam.targetEntity = static_cast<uint32_t>(targetInt);
			cam.followSpeed = followSpeedCopy;
		}
		else if (activeNow) {
			cam.targetEntity = static_cast<uint32_t>(targetInt);
			cam.followSpeed = followSpeedCopy;
		}

		ImGui::Separator();
		ImGui::SeparatorText("Map Bounds");

		// --- Use Map Bounds ---
		bool useBoundsCopy = cam.useMapBounds;
		if (ImGui::Checkbox("Use Map Bounds", &useBoundsCopy)) {
			std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			cam.useMapBounds = useBoundsCopy;
			std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
			if (beforeJSON != afterJSON)
				historyManager_.RecordModifyComponent(entity, "Modified Camera UseMapBounds", beforeJSON, afterJSON, compType);
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Clamps the camera position so it never shows beyond the map boundaries.");

		if (cam.useMapBounds) {
			if (ImGui::DragFloat("Min X", &cam.mapMinX, 1.0f, -10000.0f, cam.mapMaxX, "%.1f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::DragFloat("Max X", &cam.mapMaxX, 1.0f, cam.mapMinX, 10000.0f, "%.1f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::DragFloat("Min Y", &cam.mapMinY, 1.0f, -10000.0f, cam.mapMaxY, "%.1f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;
			if (ImGui::DragFloat("Max Y", &cam.mapMaxY, 1.0f, cam.mapMinY, 10000.0f, "%.1f")) activeNow = true;
			if (ImGui::IsItemActive()) activeNow = true;

			ImGui::Spacing();
			if (ImGui::Button("Auto-Fit Bounds from Platforms")) {
				std::string beforeJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (cameraSystem_) cameraSystem_->CalculateAndSetBoundsFromPlatforms(entity);
				std::string afterJSON = EntitySerializer::SerializeComponentToString(entity, compType, *ecs);
				if (beforeJSON != afterJSON)
					historyManager_.RecordModifyComponent(entity, "Auto-Fit Camera Bounds", beforeJSON, afterJSON, compType);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Automatically calculates map bounds from all platform entities in the scene.");
		}

		// History tracking
		if (activeNow && !editSession.wasItemActive)
			editSession.Begin(entity, compType);

		if (!activeNow && editSession.wasItemActive) {
			std::stringstream ss;
			ss << "Modified Camera in " << entityMgr.GetProperties(entity).name << std::endl;
			editSession.End(entity, compType, ss.str(), historyManager_);
		}

		editSession.wasItemActive = activeNow;
	}

#pragma endregion

}
