#pragma once
#include "ScriptableEntity.h"

using namespace Framework;

class TimerFinishTriggerScript : public ScriptableEntity {
protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;
private:
    bool m_fired = false;
};