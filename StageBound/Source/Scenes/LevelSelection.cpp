/*****************************************************************//**
 * \file	LevelSelection.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	2 February 2025
 *********************************************************************/
#include "pch.h"
#include "LevelSelection.h"

#include "Entity.h"
#include "Vector2D.h"
#include "OwalaTime.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "InputSystem.h"
#include "SaveManager.h" // NativeScript
#include "TransitionController.h"
#include "Logger.h"
#include "FirebaseManager.h"
#include "PlayerNameManager.h"
#include "GameStateFlags.h"
#pragma region LOCAL LEVEL FUNCTIONS
namespace {

	struct LevelEntry {
		int index = 1;
		std::string displayName;
		std::string sceneName;
		std::string bannerTextureKey;
	};

	Entity leaderboardTextEntity = 0;

	struct UIRefs {
		// Buttons
		Entity arrowLeft = 0, arrowRight = 0;
		Entity backButton = 0, selectButton = 0;
		Entity leaderboardButton = 0;
		Entity leaderboardPanel = 0;

		// Tickets
		Entity ticketPrev = 0, ticketCurrent = 0, ticketNext = 0;

		// Dynamic Texts
		Entity levelText = 0;

		// Name entry (optional - game works fine without it)
		Entity nameEntryOverlay = 0;
		Entity nameEntryBG = 0;
		// Lighting
		Entity shadowOverlay = 0;
		std::vector<Entity> spotlights; // Optional Entity, does not need to be validated

		bool ValidateRef() const {
			return arrowLeft != 0 && arrowRight != 0 && backButton != 0 && selectButton != 0
				&& ticketPrev != 0 && ticketCurrent != 0 && ticketNext != 0
				&& levelText != 0 && shadowOverlay != 0;
		}
	};

	struct EntityState {
		Entity entity;
		bool wasActive;
	};

	// Animation State Machine
	struct CarouselAnim {
		bool isAnimating = false;
		int direction = 0;
		float timer = 0.f;
		float duration = 0.3f;

		Vector2D leftPos, centerPos, rightPos;
		Vector2D sideScale, centerScale;
	};

	// Caching
	UIRefs ui;
	SceneManagement::SceneGUID mainMenuID = 0;
	std::vector<EntityState> hiddenMenuEntities;
	bool cached = false;

	// State Tracking
	std::vector<LevelEntry> levelData;
	int currentIndex = 0;
	SaveManager* save = nullptr;
	bool leaderboardOpen = false;

	// Name entry state
	bool        nameEntryActive = false;
	bool        nameEntryJustOpened = false;
	bool        recordPromptActive = false;
	bool        recordPromptJustOpened = false;
	std::string nameInput;
	std::string namePendingScene;
	//static bool shouldRecordTime = false;

	// Use InputSystem directly - this respects keyboardLocked_ and uses GLFW state

	// Push text to the NameEntryOverlay TextComponent
	void SetNameOverlayText(Framework::EntityManager& entityMgr, const std::string& str) {
		using namespace Framework;
		if (!entityMgr.ValidateEntity(ui.nameEntryOverlay)) return;
		if (!entityMgr.HasComponent(ui.nameEntryOverlay, TextComponentType)) return;
		TextComponent& tc = entityMgr.GetComponent<TextComponent>(ui.nameEntryOverlay);
		tc.textToDraw = str;
		tc.isDirty = true;
	}

	// Forward declaration
	void OpenNameEntry(Framework::EntityManager& entityMgr, const std::string& scene);

	// Show the "record your time?" prompt using the same overlay entity
	void OpenRecordPrompt(Framework::EntityManager& entityMgr, const std::string& scene) {
		recordPromptActive = true;
		recordPromptJustOpened = true;
		namePendingScene = scene;

		GameManager::GetInstance().GetSystems().input_->SetKeyboardLocked(true);

		if (entityMgr.ValidateEntity(ui.nameEntryBG))
			entityMgr.GetProperties(ui.nameEntryBG).isActive = true;
		if (entityMgr.ValidateEntity(ui.nameEntryOverlay))
			entityMgr.GetProperties(ui.nameEntryOverlay).isActive = true;

		SetNameOverlayText(entityMgr, "record your time?\n(y) yes   (n) no");
	}

	void CloseRecordPrompt(Framework::EntityManager& entityMgr) {
		recordPromptActive = false;
		GameManager::GetInstance().GetSystems().input_->SetKeyboardLocked(false);

		if (entityMgr.ValidateEntity(ui.nameEntryBG))
			entityMgr.GetProperties(ui.nameEntryBG).isActive = false;
		if (entityMgr.ValidateEntity(ui.nameEntryOverlay))
			entityMgr.GetProperties(ui.nameEntryOverlay).isActive = false;
	}

	void UpdateRecordPrompt(Framework::EntityManager& entityMgr) {
		GLFWwindow* win = GameManager::GetInstance().GetSystems().win_->window();
		if (!win) return;

		struct KeyState { bool prev = false; };
		static KeyState yKey, nKey;

		if (recordPromptJustOpened) {
			yKey = nKey = {};
			recordPromptJustOpened = false;
		}

		auto Just = [](KeyState& ks, int glfwKey, GLFWwindow* w) -> bool {
			bool cur = (glfwGetKey(w, glfwKey) == GLFW_PRESS);
			bool just = cur && !ks.prev;
			ks.prev = cur;
			return just;
			};

		if (Just(yKey, GLFW_KEY_Y, win)) {
			// Player wants to record
			GameStateFlags::shouldRecordTime = true;
			Logger::Get().Info(DebugType::GameManager)
				<< "[LevelSelection] *** PLAYER PRESSED Y - shouldRecordTime set to TRUE ***" << std::endl;
			CloseRecordPrompt(entityMgr);
			OpenNameEntry(entityMgr, namePendingScene);
			return;
		}

		if (Just(nKey, GLFW_KEY_N, win)) {
			// Player doesn't want to record
			GameStateFlags::shouldRecordTime = false;  // Clear the shared flag
			CloseRecordPrompt(entityMgr);

			// Load the level directly
			GameManager::GetInstance().InitiateLevelLoad(namePendingScene);
			return;
		}
	}

	// Show overlay panel + text, store which scene to load
// Show overlay panel + text, store which scene to load
	void OpenNameEntry(Framework::EntityManager& entityMgr, const std::string& scene) {
		Logger::Get().Info(DebugType::GameManager)
			<< "[LevelSelection] Opening name entry for scene: " << scene << std::endl;
		Logger::Get().Info(DebugType::GameManager)
			<< "[LevelSelection] Current shouldRecordTime: " << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

		nameEntryActive = true;
		nameEntryJustOpened = true;
		namePendingScene = scene;
		nameInput = "";  // Start with empty name

		// CRITICAL: Flush the keyboard state to prevent the 'Y' key from being carried over
		GLFWwindow* win = GameManager::GetInstance().GetSystems().win_->window();
		if (win) {
			// Read all key states to clear any pending key presses
			for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++) {
				glfwGetKey(win, i);  // This just reads and clears the state
			}
		}

		GameManager::GetInstance().GetSystems().input_->SetKeyboardLocked(true);

		if (entityMgr.ValidateEntity(ui.nameEntryBG))
			entityMgr.GetProperties(ui.nameEntryBG).isActive = true;
		if (entityMgr.ValidateEntity(ui.nameEntryOverlay))
			entityMgr.GetProperties(ui.nameEntryOverlay).isActive = true;

		SetNameOverlayText(entityMgr, "enter your name:\n" + nameInput + "_");

		Logger::Get().Info(DebugType::GameManager)
			<< "[NameEntry] Opened for: " << scene << std::endl;
	}

	// Hide overlay, clear state
	void CloseNameEntry(Framework::EntityManager& entityMgr) {
		nameEntryActive = false;
		GameManager::GetInstance().GetSystems().input_->SetKeyboardLocked(false);

		if (entityMgr.ValidateEntity(ui.nameEntryBG))
			entityMgr.GetProperties(ui.nameEntryBG).isActive = false;
		if (entityMgr.ValidateEntity(ui.nameEntryOverlay))
			entityMgr.GetProperties(ui.nameEntryOverlay).isActive = false;
	}

	// Called every frame while nameEntryActive is true - handles typing.
	// Uses SetKeyboardLocked(true) to silence InputSystem::Update() global shortcuts
	// (N=scene switch, R=camera reset, etc.) and reads GLFW directly since
	// keyboardLocked_ also blocks InputSystem::KeyPressed.
	void UpdateNameEntry(Framework::EntityManager& entityMgr) {
		GLFWwindow* win = GameManager::GetInstance().GetSystems().win_->window();
		if (!win) return;

		struct KeyState { bool prev = false; };
		static KeyState enter, escape, backspace, space;
		static KeyState letters[26], digits[10];

		if (nameEntryJustOpened) {
			enter = escape = backspace = space = {};
			for (auto& k : letters) k = {};
			for (auto& k : digits)  k = {};
			nameEntryJustOpened = false;
		}

		auto Just = [](KeyState& ks, int glfwKey, GLFWwindow* w) -> bool {
			bool cur = (glfwGetKey(w, glfwKey) == GLFW_PRESS);
			bool just = cur && !ks.prev;
			ks.prev = cur;
			return just;
			};

		// Confirm
		if (Just(enter, GLFW_KEY_ENTER, win)) {
			if (nameInput.empty()) {
				SetNameOverlayText(entityMgr, "enter your name:\n_\n(cannot be empty)");
				return;
			}

			Logger::Get().Info(DebugType::GameManager)
				<< "[NameEntry] Player entered name: '" << nameInput << "'" << std::endl;
			Logger::Get().Info(DebugType::GameManager)
				<< "[NameEntry] shouldRecordTime BEFORE save: " << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

			// Save the name
			Framework::PlayerNameManager::Get().SetName(nameInput);

			Logger::Get().Info(DebugType::GameManager)
				<< "[NameEntry] Saved: \"" << nameInput << "\", loading: " << namePendingScene << std::endl;
			Logger::Get().Info(DebugType::GameManager)
				<< "[NameEntry] shouldRecordTime AFTER save: " << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

			CloseNameEntry(entityMgr);
			GameManager::GetInstance().InitiateLevelLoad(namePendingScene);
			return;
		}

		// Cancel
		if (Just(escape, GLFW_KEY_ESCAPE, win)) {
			CloseNameEntry(entityMgr);
			return;
		}

		bool changed = false;

		// Backspace
		if (Just(backspace, GLFW_KEY_BACKSPACE, win)) {
			if (!nameInput.empty()) { nameInput.pop_back(); changed = true; }
		}
		else {
			// A-Z always uppercase
			for (int i = 0; i < 26; ++i) {
				if (Just(letters[i], GLFW_KEY_A + i, win) && nameInput.size() < 20) {
					nameInput += static_cast<char>('a' + i);
					changed = true;
					break;
				}
			}
			// 0-9
			for (int i = 0; i < 10; ++i) {
				if (Just(digits[i], GLFW_KEY_0 + i, win) && nameInput.size() < 20) {
					nameInput += static_cast<char>('0' + i);
					changed = true;
					break;
				}
			}
			// Space
			if (Just(space, GLFW_KEY_SPACE, win) && nameInput.size() < 20) {
				nameInput += ' ';
				changed = true;
			}
		}

		if (changed)
			SetNameOverlayText(entityMgr, "enter your name:\n" + nameInput + "_");
	}
	
	// Animation Tracking
	CarouselAnim carousel;

	bool ConsumeClick(Framework::EntityManager& entityMgr, Framework::InputSystem& inputSystem, Entity entity) {
		if (!entityMgr.ValidateEntity(entity)) return false;
		if (!entityMgr.HasComponent(entity, Framework::ButtonType)) return false;

		Framework::Button& button = entityMgr.GetComponent<Framework::Button>(entity);
		if (button.isHovered && inputSystem.MouseReleased(InputSystem::MouseButton::Left)) {
			return true;
		}
		return false;
	}

	void CacheUI(Framework::EntityManager& entityMgr) {
		using namespace Framework;

		const std::vector<Entity>& entities = SceneManager::GetInstance().GetEntitiesInScene("LevelSelection");
		for (const Entity entity : entities) {
			const std::string& name = entityMgr.GetProperties(entity).name;

			if (name == "ArrowLeft")          ui.arrowLeft = entity;
			else if (name == "ArrowRight")         ui.arrowRight = entity;
			else if (name == "BackButton")         ui.backButton = entity;
			else if (name == "SelectLevelButton")  ui.selectButton = entity;
			else if (name == "Ticket_Prev")        ui.ticketPrev = entity;
			else if (name == "Ticket_Current")     ui.ticketCurrent = entity;
			else if (name == "Ticket_Next")        ui.ticketNext = entity;
			else if (name == "LevelText")          ui.levelText = entity;
			else if (name == "LeaderboardText")    leaderboardTextEntity = entity;
			else if (name == "LeaderboardButton")  ui.leaderboardButton = entity;
			else if (name == "LeaderboardPanel")   ui.leaderboardPanel = entity;
			else if (name == "NameEntryOverlay")   ui.nameEntryOverlay = entity;
			else if (name == "NameEntryOverlayBG") ui.nameEntryBG = entity;
			else if (name == "ShadowOverlay") ui.shadowOverlay = entity;
			else if (name == "Spotlight") ui.spotlights.push_back(entity);
			else if (name == "SpotlightShadow") ui.spotlights.push_back(entity);
		}

		// Only the core UI must be present - name entry is optional
		if (!ui.ValidateRef()) {
			Logger::Get().Info(DebugType::GameManager) << "Not all UI elements in LevelSelection Scene was cached!" << std::endl;
			return;
		}

		// Cache Base Positions
		carousel.leftPos = entityMgr.GetComponent<Transform>(ui.ticketPrev).position;
		carousel.centerPos = entityMgr.GetComponent<Transform>(ui.ticketCurrent).position;
		carousel.rightPos = entityMgr.GetComponent<Transform>(ui.ticketNext).position;

		// Cache Base Scales
		carousel.sideScale = entityMgr.GetComponent<TextureComponent>(ui.ticketPrev).HalfSize;
		carousel.centerScale = entityMgr.GetComponent<TextureComponent>(ui.ticketCurrent).HalfSize;

		cached = true;
	}

	void SetLeaderboardVisible(Framework::EntityManager& entityMgr, bool visible) {
		if (entityMgr.ValidateEntity(leaderboardTextEntity))
			entityMgr.GetProperties(leaderboardTextEntity).isActive = visible;
		if (entityMgr.ValidateEntity(ui.leaderboardPanel))
			entityMgr.GetProperties(ui.leaderboardPanel).isActive = visible;
	}

	void RefreshLeaderboard() {
		if (levelData.empty()) return;
		std::string levelName = levelData[currentIndex].sceneName;

		FirebaseManager::Get().FetchLeaderboard(levelName,
			[](std::vector<Framework::LeaderboardEntry> entries) {
				EntityManager mgr(GameManager::GetInstance().GetECS());
				if (!mgr.ValidateEntity(leaderboardTextEntity)) return;
				if (!mgr.HasComponent(leaderboardTextEntity, Framework::TextComponentType)) return;

				Framework::TextComponent& text = mgr.GetComponent<Framework::TextComponent>(leaderboardTextEntity);

				if (entries.empty()) {
					text.textToDraw = "BEST TIMES\n===========\nno records yet";
					text.isDirty = true;
					return;
				}

				std::string display = "BEST TIMES\n===========\n# - NAME - TIME\n-----------\n";

				int rank = 1;
				for (auto& e : entries) {
					char buf[128];
					std::string name = e.name.size() > 10 ? e.name.substr(0, 10) : e.name;
					std::snprintf(buf, sizeof(buf), "%d - %s - %.2fs\n", rank++, name.c_str(), e.time);
					display += buf;
					if (rank > 10) break;
				}

				display += "===========";
				text.textToDraw = display;
				text.isDirty = true;
			});
	}

	SaveManager* GetSaveManager(Framework::EntityManager& entityMgr) {
		using namespace Framework;

		const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
		for (Entity entity : persistents) {
			if (!entityMgr.ValidateEntity(entity)) continue;
			if (SaveManager* sm = entityMgr.GetScript<SaveManager>(entity)) {
				return sm;
			}
		}
		return nullptr;
	}


	void SetTicketTexture(Framework::EntityManager& entityMgr, Entity ticket, const std::string& key) {
		using namespace Framework;

		if (!entityMgr.ValidateEntity(ticket)) return;
		if (!entityMgr.HasComponent(ticket, TextureComponentType)) return;

		entityMgr.GetComponent<TextureComponent>(ticket).textureKey = key;
	}

	void SetLevelText(Framework::EntityManager& entityMgr, const std::string& value) {
		using namespace Framework;

		if (!entityMgr.ValidateEntity(ui.levelText)) return;
		if (!entityMgr.HasComponent(ui.levelText, TextComponentType)) return;

		TextComponent& text = entityMgr.GetComponent<TextComponent>(ui.levelText);
		text.textToDraw = value;
		text.isDirty = true;
	}

	std::string GetTicketTextureForIndex(int index) {
		if (levelData.empty()) return "";

		if (save && save->IsUnlocked(levelData[index].index)) {
			return levelData[index].bannerTextureKey;
		}

		if (!GameManager::GetInstance().IsGamePlaying() || index == 0) {
			return levelData[0].bannerTextureKey;
		}

		return "SB_Locked.png";
	}

	void RefreshUI(Framework::EntityManager& entityMgr) {
		if (levelData.empty()) return;

		int n = static_cast<int>(levelData.size());
		int prevIndex = (currentIndex - 1 + n) % n;
		int nextIndex = (currentIndex + 1) % n;

		// Left Ticket
		SetTicketTexture(entityMgr, ui.ticketPrev, "SB_LevelSelection_EmptyTicket.png");
		// Middle Ticket
		SetTicketTexture(entityMgr, ui.ticketCurrent, GetTicketTextureForIndex(currentIndex));
		// Right Ticket
		SetTicketTexture(entityMgr, ui.ticketNext, "SB_LevelSelection_EmptyTicket.png");

		if (save && save->IsUnlocked(levelData[currentIndex].index)) {
			SetLevelText(entityMgr, levelData[currentIndex].displayName);
		}
		else {
			SetLevelText(entityMgr, (!GameManager::GetInstance().IsGamePlaying() || currentIndex == 0) ? levelData[0].displayName : "locked");
		}
	}

	static void HideMenuEntities(Framework::EntityManager& entityMgr) {
		using namespace Framework;

		hiddenMenuEntities.clear();

		// Find MainMenu Scene and cache it.
		SceneManager& sceneMgr = SceneManager::GetInstance();
		if (mainMenuID == SceneManager::GetInvalidSceneID()) {
			for (SceneManagement::SceneGUID sceneID : sceneMgr.GetLoadedScenes()) {
				const SceneAsset& scene = sceneMgr.GetSceneAt(sceneID, true);
				if (scene.IsValid() && scene.name == "MainMenu") {
					mainMenuID = sceneID;
					break;
				}
			}
		}

		if (mainMenuID == SceneManager::GetInvalidSceneID()) return;

		const std::vector<Entity> menuEntities = sceneMgr.GetEntitiesInScene(mainMenuID);

		hiddenMenuEntities.reserve(menuEntities.size());

		for (Entity entity : menuEntities) {
			if (!entityMgr.ValidateEntity(entity)) continue;

			EntityProperties& props = entityMgr.GetProperties(entity);

			// Record previous state of the entity
			hiddenMenuEntities.push_back({ entity, props.isActive });

			// Hide the entity
			props.isActive = false;
		}
	}

	void ToggleLighting(Framework::EntityManager& entityMgr, bool value) {
		// If light is on, text is shown
		entityMgr.SetActive(ui.levelText, value);
		// If light is on, shadow is off
		entityMgr.SetActive(ui.shadowOverlay, !value);
		// If light is on, spotlight is on
		for (Entity spotlight : ui.spotlights) {
			entityMgr.SetActive(spotlight, value);
		}
	}

	void HandleSceneKBInputs(Framework::InputSystem& input) {
		if (!save) return;
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		if (input.KeyPressed(InputSystem::Key::Num1)) {
			save->SetUnlock(1);
			RefreshUI(entityMgr);
		}
		if (input.KeyPressed(InputSystem::Key::Num2)) {
			save->SetUnlock(2);
			RefreshUI(entityMgr);
		}
		if (input.KeyPressed(InputSystem::Key::Num3)) {
			save->SetUnlock(3);
			RefreshUI(entityMgr);
		}
		if (input.KeyPressed(InputSystem::Key::Num4)) {
			save->SetUnlock(4);
			RefreshUI(entityMgr);
		}
	}

	static void RestoreMainMenuEntities(Framework::EntityManager& entityMgr) {
		for (const EntityState& entity : hiddenMenuEntities) {
			// Check if entity still exists.
			if (!entityMgr.ValidateEntity(entity.entity)) continue;
			entityMgr.GetProperties(entity.entity).isActive = entity.wasActive;
		}
		hiddenMenuEntities.clear();
	}
}
#pragma endregion

namespace LevelSelection {
	using namespace Framework;

	void Initialize() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		CacheUI(entityMgr);
		save = GetSaveManager(entityMgr);

		Framework::PlayerNameManager::Get().Load();

		bool shouldSkipIntro = (save && save->GetSkipIntro()) ? true : false;
		std::string firstLevel = shouldSkipIntro ? "Tutorial 1" : "Cutscene Intro";

		levelData = {
			{1, "act i: the forest", firstLevel, "SB__ForestTicketGlow.png"},
			{2, "act ii: the hut", "Tutorial 2", "SB__WitchHutTicketGlow.png"},
			{3, "act iii: workshop", "Level 1", "SB__WorkshopTicketGlow.png"},
			{4, "act iv: backstage", "Level 2", "SB__BackstageTicketGlow.png"}
		};

		HideMenuEntities(entityMgr);

		currentIndex = 0;
		leaderboardOpen = false;
		nameEntryActive = false;
		recordPromptActive = false;
		nameInput.clear();

		// Ensure overlay starts hidden regardless of JSON state
		CloseNameEntry(entityMgr);

		SetLeaderboardVisible(entityMgr, false);
		RefreshUI(entityMgr);

		// Reveal the scene � it was loaded while black due to the prior FadeOut in InitiateLevelLoad.
		GameManager::GetInstance().FadeIn(0.5f);
	}


	void ResetFade() {
		Logger::Get().Info(DebugType::GameManager) << "LevelSelection fade reset" << std::endl;
	}

	void Update() {
		FirebaseManager::Get().Update();
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		InputSystem& inputSystem = *GameManager::GetInstance().GetSystems().input_;

		if (!cached) CacheUI(entityMgr);
		if (levelData.empty()) return;

		// While record prompt is open, only process Y/N
		if (recordPromptActive) {
			UpdateRecordPrompt(entityMgr);
			return;
		}

		// While name entry is open, only process typing � don't block on fading
		if (nameEntryActive) {
			UpdateNameEntry(entityMgr);
			return;
		}

		// Block navigation input while a fade is running
		if (GameManager::GetInstance().IsFading()) return;

		// Keyboard Input Handling
		HandleSceneKBInputs(inputSystem);

		// Level Selection Carousel
		int n = static_cast<int>(levelData.size());

		if (!carousel.isAnimating) {
			// Left Arrow Clicked
			if (ConsumeClick(entityMgr, inputSystem, ui.arrowLeft)) {
				int prevPrevIndex = (currentIndex - 2 + n) % n;

				carousel.isAnimating = true;
				carousel.direction = 1;
				carousel.timer = 0.f;
				ToggleLighting(entityMgr, false);

				// Sets all tickets to blank
				SetTicketTexture(entityMgr, ui.ticketPrev, "SB_LevelSelection_EmptyTicket.png");
				SetTicketTexture(entityMgr, ui.ticketCurrent, "SB_LevelSelection_EmptyTicket.png");
				SetTicketTexture(entityMgr, ui.ticketNext, "SB_LevelSelection_EmptyTicket.png");
			}

			// Right Arrow Clicked
			if (ConsumeClick(entityMgr, inputSystem, ui.arrowRight)) {
				int nextNextIndex = (currentIndex + 2) % n;
				SetTicketTexture(entityMgr, ui.ticketPrev, GetTicketTextureForIndex(nextNextIndex));

				carousel.isAnimating = true;
				carousel.direction = -1;
				carousel.timer = 0.f;
				ToggleLighting(entityMgr, false);
				SetTicketTexture(entityMgr, ui.ticketPrev, "SB_LevelSelection_EmptyTicket.png");
				SetTicketTexture(entityMgr, ui.ticketCurrent, "SB_LevelSelection_EmptyTicket.png");
				SetTicketTexture(entityMgr, ui.ticketNext, "SB_LevelSelection_EmptyTicket.png");
			}
		}

		if (carousel.isAnimating) {
			carousel.timer += Time::DeltaTime();
			float t = std::min(carousel.timer / carousel.duration, 1.f);

			float easeT = 1.f - std::pow(1.f - t, 3.f);

			// Relative Positions
			Transform& leftTransform = entityMgr.GetComponent<Transform>(ui.ticketPrev);
			Transform& centerTransform = entityMgr.GetComponent<Transform>(ui.ticketCurrent);
			Transform& rightTransform = entityMgr.GetComponent<Transform>(ui.ticketNext);

			// Relative Scales
			Vector2D& leftScale = entityMgr.GetComponent<TextureComponent>(ui.ticketPrev).HalfSize;
			Vector2D& centerScale = entityMgr.GetComponent<TextureComponent>(ui.ticketCurrent).HalfSize;
			Vector2D& rightScale = entityMgr.GetComponent<TextureComponent>(ui.ticketNext).HalfSize;

			// Linear Interp lambda for animation, might move this to a utility class
			float offsetX = carousel.rightPos.x - carousel.centerPos.x;

			// Offscreen positions for the illusion of wrapping
			Vector2D farLeftPos = { carousel.leftPos.x - offsetX, carousel.leftPos.y };
			Vector2D farRightPos = { carousel.rightPos.x + offsetX, carousel.rightPos.y };

			// All tickets are shrunk to normal size when carousel is animating
			leftScale = centerScale = rightScale = carousel.sideScale;

			if (carousel.direction == -1) {
				// Center Ticket moves down-left to the left position and shrinks
				centerTransform.position.x = std::lerp(carousel.centerPos.x, carousel.leftPos.x, easeT);
				centerTransform.position.y = std::lerp(carousel.centerPos.y, carousel.leftPos.y, easeT);

				// Right Ticket moves up-left to the center position and grows
				rightTransform.position.x = std::lerp(carousel.rightPos.x, carousel.centerPos.x, easeT);
				rightTransform.position.y = std::lerp(carousel.rightPos.y, carousel.centerPos.y, easeT);

				// Left Ticket wraps around and moves in from offscreen to the right position
				leftTransform.position.x = std::lerp(farRightPos.x, carousel.rightPos.x, easeT);
				leftTransform.position.y = std::lerp(farRightPos.y, carousel.rightPos.y, easeT);
			}
			else {
				// Center Ticket moves down-right to the left position and shrinks
				centerTransform.position.x = std::lerp(carousel.centerPos.x, carousel.rightPos.x, easeT);
				centerTransform.position.y = std::lerp(carousel.centerPos.y, carousel.rightPos.y, easeT);

				// Left Ticket moves up-right to the center position and grows
				leftTransform.position.x = std::lerp(carousel.leftPos.x, carousel.centerPos.x, easeT);
				leftTransform.position.y = std::lerp(carousel.leftPos.y, carousel.centerPos.y, easeT);;

				// Right Ticket wraps around and moves in from offscreen to the left position
				rightTransform.position.x = std::lerp(farLeftPos.x, carousel.leftPos.x, easeT);
				rightTransform.position.y = std::lerp(farLeftPos.y, carousel.leftPos.y, easeT);
			}

			if (t >= 1.f) {
				carousel.isAnimating = false;

				// Snap back to original transforms
				leftTransform.position = carousel.leftPos;
				centerTransform.position = carousel.centerPos;
				rightTransform.position = carousel.rightPos;

				leftScale = carousel.sideScale;
				centerScale = carousel.centerScale;
				rightScale = carousel.sideScale;

				ToggleLighting(entityMgr, true);

				// Update Index
				if (carousel.direction == -1) {
					currentIndex = (currentIndex + 1) % n;
				}
				else {
					currentIndex = (currentIndex - 1 + n) % n;
				}

				RefreshUI(entityMgr);
			}
		}

		// Return to Main Menu Button
		if (ConsumeClick(entityMgr, inputSystem, ui.backButton)) {
			SceneManager& sceneMgr = SceneManager::GetInstance();
			bool mainMenuLoaded = false;

			for (SceneManagement::SceneGUID sceneID : sceneMgr.GetLoadedScenes()) {
				if (sceneMgr.GetSceneAt(sceneID, true).name == "MainMenu") {
					mainMenuLoaded = true;
					break;
				}
			}

			if (mainMenuLoaded) {
				sceneMgr.UnloadScene("LevelSelection");
			}
			else {
				GameManager::GetInstance().QueueLevelLoad("MainMenu");
			}
			return;
		}

		if (ConsumeClick(entityMgr, inputSystem, ui.leaderboardButton)) {
			leaderboardOpen = !leaderboardOpen;
			SetLeaderboardVisible(entityMgr, leaderboardOpen);
			if (leaderboardOpen) {
				auto& t = entityMgr.GetComponent<Framework::TextComponent>(leaderboardTextEntity);
				t.textToDraw = "Loading...";
				t.isDirty = true;
				RefreshLeaderboard();
			}
		}

		// Select Button
		if (ConsumeClick(entityMgr, inputSystem, ui.selectButton)) {
			if (save && entityMgr.ValidateEntity(save->GetEntity())) {
				const int levelIndex = levelData[currentIndex].index;
				if (!save->IsUnlocked(levelIndex) || carousel.isAnimating) return;
			}
			const std::string scene = levelData[currentIndex].sceneName;

			// Set the current level name in game state flags
			GameStateFlags::currentLevelName = scene;

			// Ask player if they want their time recorded before loading the level.
			if (entityMgr.ValidateEntity(ui.nameEntryOverlay)) {
				OpenRecordPrompt(entityMgr, scene);
			}
			else {
				GameManager::GetInstance().InitiateLevelLoad(scene);
			}
		}
	}

	void Free() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		RestoreMainMenuEntities(entityMgr);

		levelData.clear();
		leaderboardOpen = false;
		nameEntryActive = false;
		recordPromptActive = false;
		nameInput.clear();
		namePendingScene.clear();
		currentIndex = 0;
		cached = false;
		ui = {};
		save = nullptr;
		leaderboardTextEntity = 0;

		// Reset game state flags
		// 
	}
}