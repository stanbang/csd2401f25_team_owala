#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class FPS : public ScriptableEntity {
private:
	bool shouldHide = true;
	float accum_ = 0.f;
	int frames_ = 0;
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
