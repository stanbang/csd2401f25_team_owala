#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class LoopingSoundScript : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
