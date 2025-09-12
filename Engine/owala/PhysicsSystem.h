#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Collision.h"
#include <vector>

namespace Framework {

    class PhysicsSystem : public ISystem {
    public:
        explicit PhysicsSystem(EntityStorage& storage);

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        Vector2D gravity_ = { 0.0f, -9.81f };

        // Public method to control the physics simulation
        void SetPaused(bool paused) { isPaused_ = paused; }

    private:
        void Step(float dt);
        void Integrate(float dt);
        void DetectCollisions(float dt);
        void ResolveCollisions(float dt);

        EntityStorage* ecs_;
        float timeAccumulator_ = 0.0f;

        // Flag to pause physics
        bool isPaused_ = false;

        
        std::vector<Contact> contacts_;
    };

} // namespace Framework