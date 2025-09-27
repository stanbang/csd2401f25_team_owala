#pragma once
#include "ISystem.h"
#include "EntityStorage.h"

namespace Framework {
    class TrapPlatformSystem : public ISystem {
    private:
        EntityStorage* ecs_;

    public:
        TrapPlatformSystem(EntityStorage& storage) : ecs_(&storage) {}

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}
    };
}