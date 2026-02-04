#pragma once

#include "ScriptableEntity.h"
#include "Entity.h"
#include "SaveManager.h"

using namespace Framework;

class Portal : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
private:
	Entity player = 0;
	SaveManager* save = nullptr;
	int levelIndexToUnlock = 2;

	void SearchForPlayer();
	void SearchForSaveManager();
};
