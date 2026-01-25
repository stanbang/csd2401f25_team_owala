#pragma once

#include "ScriptableEntity.h"
#include "GameManager.h"
using namespace Framework;

class OptionsMenuBackButton : public ScriptableEntity {
private:
	bool shouldCloseMenu_;
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
