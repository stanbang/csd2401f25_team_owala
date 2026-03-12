/*****************************************************************//**
 * \file	CutsceneController.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the Cutscene Controller.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "CutsceneController.h"
#include "ScriptRegistry.h"

// Dependencies
#include "GameManager.h"
#include "EntityManager.h"
#include "SaveManager.h"
#include "AnimationSystem.h"
#include "InputSystem.h"
#include "Components.h"
#include "OwalaTime.h"
#include "Entity.h"
#include "WindowsSystem.h"

REGISTER_SCRIPT(CutsceneController, "CutsceneController")

void CutsceneController::ApplySlide() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());
	
	// Get reference to self
	Entity self = GetEntity();

	TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(self);
	texture.textureKey = slides[currentSlide].textureKey;
	AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(self);
	AnimationSystem::ResetAnimation(animComp);

	timer = 0.f;
}

bool CutsceneController::NextSlide() {
	currentSlide++;
	if (currentSlide >= static_cast<int>(slides.size())) {
		FinishCutscene();
		return true;
	}

	ApplySlide();
	return false;
}

void CutsceneController::FinishCutscene() {
	if (isEnding) return;
	isEnding = true;

	EntityManager entityMgr(GameManager::GetInstance().GetECS());
	
	// Search for SaveManager
	const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
	for (Entity entity : persistents) {
		if (!entityMgr.ValidateEntity(entity)) continue;
		if (SaveManager* save = entityMgr.GetScript<SaveManager>(entity)) {
			save->SetSkipIntro(true);
			break;
		}
	}
	
	GameManager::GetInstance().InitiateLevelLoad("Tutorial 1");
}

void CutsceneController::FitImageToScreen() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());
	//WindowsSystem& win = *GameManager::GetInstance().GetSystems().win_;

	Entity self = GetEntity();
	TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(self);
	Transform& transform = entityMgr.GetComponent<Transform>(self);

	texture.HalfSize = { 1.f, 1.f };
	transform.position = { 0.f, 0.f };
}

bool CutsceneController::IsSkipHeldDown() const {
	InputSystem* input = GameManager::GetInstance().GetSystems().input_;
	if (!input) return false;

	return input->KeyDown(InputSystem::Key::Space) || input->KeyDown(InputSystem::Key::F);
}

void CutsceneController::OnCreate() {
	slides = {
		{"SB_CutScene_01_Spritesheet01.png", 2.f},
		{"SB_CutScene_02_Spritesheet01.png", 1.6f},
		{"Cut_Scene_3_Spritesheet_01.png", 2.f},
		{"Cut_Scene_4_Spritesheet_01.png", 2.5f},
	};

	currentSlide = 0;
	timer = 0.f;

	skipHoldTime = 0.f;
	wasSkipDown = skipTriggered = false;

	ApplySlide();
}

void CutsceneController::OnUpdate() {
	if (isEnding) return;

	//GameManager& gm = GameManager::GetInstance();

	FitImageToScreen();

	bool skipDown = IsSkipHeldDown();

	if (skipDown) {
		skipHoldTime += Time::DeltaTime();

		if (!skipTriggered && skipHoldTime >= 2.f) {
			skipTriggered = true;
			FinishCutscene();
			return;
		}
	}
	else {
		if (wasSkipDown && !skipTriggered) {
			if (skipHoldTime > 0.f && skipHoldTime < 2.f) {
				if (NextSlide()) return;
			}
		}

		skipHoldTime = 0.f;
		skipTriggered = false;
	}

	wasSkipDown = skipDown;

	if (!skipDown) {
		timer += Time::DeltaTime();
		if ((currentSlide < static_cast<int>(slides.size())) && (timer >= slides[currentSlide].duration)) {
			if (NextSlide()) return;
		}
	}
}

void CutsceneController::OnDestroy() {
	// Logic to be called before Entity is deleted
}
