/*****************************************************************//**
 * \file   RestartButtonScript.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Source file for restart button logic to switch to previous scene.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "RestartButtonScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(RestartButtonScript, "RestartButtonScript")

void RestartButtonScript::OnCreate() {
	self_ = GetEntity();
}

void RestartButtonScript::OnUpdate() {
	GameManager &gameMgr = GameManager::GetInstance();
	EntityManager entityMgr(gameMgr.GetECS());

	if (entityMgr.HasComponent(self_, ButtonType)) {
		Button btn = entityMgr.GetComponent<Button>(self_);
		
		// button clicked
		if (btn.isPressed) {
			SceneManager& sceneMgr = SceneManager::GetInstance();

			std::cout << "prev scene: " << gameMgr.GetPrevScene() << std::endl;
			sceneMgr.LoadScene(gameMgr.GetPrevScene());
		}
	}
}

void RestartButtonScript::OnDestroy() {
	// Logic to be called before Entity is deleted
}
