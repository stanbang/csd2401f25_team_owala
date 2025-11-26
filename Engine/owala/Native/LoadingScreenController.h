#pragma once
#include "ScriptableEntity.h"

namespace Framework {

    class LoadingScreenController : public ScriptableEntity {
    protected:
        void OnCreate() override;
        void OnUpdate() override;
        void OnDestroy() override;
    };

}