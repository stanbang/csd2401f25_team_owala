#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class StageGearRotate : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
private:
	float prevStageRotation = 0.f;
public:
	bool counterClockWise = true;
};
