
/*****************************************************************//**
 * \file   ContinueButtonScript.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Source file for continue button logic toswitch to next level scene.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "ContinueButtonScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(ContinueButtonScript, "ContinueButtonScript")

void ContinueButtonScript::OnCreate() {
	self_ = GetEntity();
}

void ContinueButtonScript::OnUpdate() {
	if (initiatedNextLevelLoad) return;

	GameManager& gameMgr = GameManager::GetInstance();
	EntityManager entityMgr(gameMgr.GetECS());

	if (entityMgr.HasComponent(self_, ButtonType)) {
		Button btn = entityMgr.GetComponent<Button>(self_);

		// button clicked
		if (btn.isPressed) {
			SceneManager& sceneMgr = SceneManager::GetInstance();
			const SceneAsset& prevScene = sceneMgr.GetSceneAt(gameMgr.GetPrevScene());
			
			std::string nextLevelName{};
			if (prevScene.name == "Tutorial 1") {
				nextLevelName = "Tutorial 2";
			}
			else if (prevScene.name == "Tutorial 2") {
				nextLevelName = "Level 1";
			} else if (prevScene.name == "Level 1") {
				nextLevelName = "Level 2";
			}

			gameMgr.InitiateLevelLoad(nextLevelName);
			initiatedNextLevelLoad = true;
		}
	}
}

void ContinueButtonScript::OnDestroy() {
	// Logic to be called before Entity is deleted
}
