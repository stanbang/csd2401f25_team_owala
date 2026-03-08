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
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "InputSystem.h"
#include "SaveManager.h" // NativeScript
#include "TransitionController.h"
#include "Logger.h"
#include "FirebaseManager.h"
#include "PlayerNameManager.h"

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

		bool ValidateRef() const {
			return arrowLeft != 0 && arrowRight != 0 && backButton != 0 && selectButton != 0
				&& ticketPrev != 0 && ticketCurrent != 0 && ticketNext != 0
				&& levelText != 0;
		}
	};

	struct EntityState {
		Entity entity;
		bool wasActive;
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
	std::string nameInput;
	std::string namePendingScene;

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

	// Show overlay panel + text, store which scene to load
	void OpenNameEntry(Framework::EntityManager& entityMgr, const std::string& scene) {
		nameEntryActive = true;
		nameEntryJustOpened = true;
		namePendingScene = scene;
		nameInput = Framework::PlayerNameManager::Get().GetName();

		// Lock so InputSystem::Update() global shortcuts (N/R/Z/X/P...) don't fire while typing
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
			Framework::PlayerNameManager::Get().SetName(nameInput);
			Logger::Get().Info(DebugType::GameManager)
				<< "[NameEntry] Saved: \"" << nameInput << "\", loading: " << namePendingScene << std::endl;
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
		}

		// Only the core UI must be present - name entry is optional
		if (!ui.ValidateRef()) {
			Logger::Get().Info(DebugType::GameManager) << "Not all UI elements in LevelSelection Scene was cached!" << std::endl;
			return;
		}

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

	void RefreshUI(Framework::EntityManager& entityMgr) {
		if (levelData.empty()) return;

		// Left Ticket
		SetTicketTexture(entityMgr, ui.ticketPrev, "SB_LevelSelection_EmptyTicket.png");

		// Middle Ticket
		if (save && save->IsUnlocked(levelData[currentIndex].index)) {
			SetTicketTexture(entityMgr, ui.ticketCurrent, levelData[currentIndex].bannerTextureKey);
			SetLevelText(entityMgr, levelData[currentIndex].displayName);
		}
		else {
			if (!GameManager::GetInstance().IsGamePlaying() || currentIndex == 0) {
				SetTicketTexture(entityMgr, ui.ticketCurrent, levelData[0].bannerTextureKey);
				SetLevelText(entityMgr, levelData[0].displayName);
			}
			else {
				SetTicketTexture(entityMgr, ui.ticketCurrent, "SB_Locked.png");
				SetLevelText(entityMgr, "locked");
			}
		}

		// Right Ticket
		SetTicketTexture(entityMgr, ui.ticketNext, "SB_LevelSelection_EmptyTicket.png");
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
		nameInput.clear();

		// Ensure overlay starts hidden regardless of JSON state
		CloseNameEntry(entityMgr);

		SetLeaderboardVisible(entityMgr, false);
		RefreshUI(entityMgr);

		// Reveal the scene — it was loaded while black due to the prior FadeOut in InitiateLevelLoad.
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

		// While name entry is open, only process typing — don't block on fading
		if (nameEntryActive) {
			UpdateNameEntry(entityMgr);
			return;
		}

		// Block navigation input while a fade is running
		if (GameManager::GetInstance().IsFading()) return;

		// Keyboard Input Handling
		HandleSceneKBInputs(inputSystem);

		// Navigation Button Handling
		if (ConsumeClick(entityMgr, inputSystem, ui.arrowLeft)) {
			const int n = static_cast<int>(levelData.size());
			currentIndex = (currentIndex - 1 + n) % n;
			RefreshUI(entityMgr);
		}
		if (ConsumeClick(entityMgr, inputSystem, ui.arrowRight)) {
			currentIndex = (currentIndex + 1) % static_cast<int>(levelData.size());
			RefreshUI(entityMgr);
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
				if (!save->IsUnlocked(levelIndex)) return;
			}
			const std::string scene = levelData[currentIndex].sceneName;

			// Always ask for a name before each run so different players on the
			// same machine get their own leaderboard entry. The overlay pre-fills
			// the last used name so returning players can just press Enter.
			// If the overlay entity is missing for any reason, fall through safely.
			if (entityMgr.ValidateEntity(ui.nameEntryOverlay)) {
				OpenNameEntry(entityMgr, scene);
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
		nameInput.clear();
		namePendingScene.clear();
		currentIndex = 0;
		cached = false;
		ui = {};
		save = nullptr;
		leaderboardTextEntity = 0;
	}
}