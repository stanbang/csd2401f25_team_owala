#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class PlayerHealthHUD : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;

private:
	Entity self_, player_;
	int lifeNum_;
	bool lifeLost_;
};
