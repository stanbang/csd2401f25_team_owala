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

#include "Logger.h"

#pragma region LOCAL LEVEL FUNCTIONS
namespace {
	struct LevelEntry {
		int index = 1;
		std::string displayName;
		std::string sceneName;
		std::string bannerTextureKey;
	};

	struct UIRefs {
		// Buttons
		Entity arrowLeft = 0, arrowRight = 0;
		Entity backButton = 0, selectButton = 0;

		// Tickets
		Entity ticketPrev = 0, ticketCurrent = 0, ticketNext = 0;

		// Dynamic Texts
		Entity levelText = 0;

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

			if (name == "ArrowLeft") ui.arrowLeft = entity;
			else if (name == "ArrowRight") ui.arrowRight = entity;
			else if (name == "BackButton") ui.backButton = entity;
			else if (name == "SelectLevelButton") ui.selectButton = entity;
			else if (name == "Ticket_Prev") ui.ticketPrev = entity;
			else if (name == "Ticket_Current") ui.ticketCurrent = entity;
			else if (name == "Ticket_Next") ui.ticketNext = entity;
			else if (name == "LevelText") ui.levelText = entity;
		}

		if (!ui.ValidateRef()) {
			Logger::Get().Info(DebugType::GameManager) << "Not all UI elements in LevelSelection Scene was cached!" << std::endl;
			return;
		}

		cached = true;
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
		RefreshUI(entityMgr);
	}

	void Update() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		InputSystem& inputSystem = *GameManager::GetInstance().GetSystems().input_;

		if (!cached) CacheUI(entityMgr);
		if (levelData.empty()) return;

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
				// Queue so LoadScene is deferred to the top of the next frame,
				// avoiding mutation of the loaded-scenes list mid-iteration.
				GameManager::GetInstance().QueueLevelLoad("MainMenu");
			}
			return;
		}

		// Select Button
		if (ConsumeClick(entityMgr, inputSystem, ui.selectButton)) {
			if (save && entityMgr.ValidateEntity(save->GetEntity())) {
				const int levelIndex = levelData[currentIndex].index;

				// If Level has not been unlocked yet
				if (!save->IsUnlocked(levelIndex)) {
					return;
				}
			}

			// Queue so LoadScene is deferred to the top of the next frame,
			// avoiding mutation of the loaded-scenes list mid-iteration.
			GameManager::GetInstance().QueueLevelLoad(levelData[currentIndex].sceneName);
			return;
		}
	}

	void Free() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		RestoreMainMenuEntities(entityMgr);

		levelData.clear();
		currentIndex = 0;
		cached = false;
		ui = {};
		save = nullptr;
	}
}