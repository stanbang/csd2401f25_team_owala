#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class PrintEntityName : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
