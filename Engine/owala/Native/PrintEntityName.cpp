/*****************************************************************//**
 * \file	PrintEntityName.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the PrintEntityName script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "PrintEntityName.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(PrintEntityName, "PrintEntityName")

void PrintEntityName::OnCreate() {
	// Initialization Code Here
}

void PrintEntityName::OnUpdate() {
	InputSystem& inputSystem = *GameManager::GetInstance().GetSystems().input_;
	EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

	if (inputSystem.KeyPressed(InputSystem::Key::M)) {
		std::cout << "Entity: My name is " << entityMgr.GetProperties(GetEntity()).name << std::endl;
	}
}

void PrintEntityName::OnDestroy() {
	// Logic to be called before Entity is deleted
}
