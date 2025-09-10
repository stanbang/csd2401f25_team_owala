#pragma once
#include "ISystem.h"
#include "Entity.hpp"
#include "Components.h"
#include <unordered_map>


namespace Framework {

    class PhysicsSystem : public ISystem {
    public:
        // Expose gravity so it can be tweaked, e.g., for world rotation.
        Vector2D gravity = { 0.0f, -9.81f };

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        // Component Management
        void AddTransform(Entity entity, Vector2D position = { 0.0f, 0.0f });
        void AddRigidBody(Entity entity, bool isStatic = false);

        TransformComponent& GetTransform(Entity entity);
        RigidBodyComponent& GetRigidBody(Entity entity);

    private:
        // Maps to store components for each entity.
        // This is a common ECS pattern where systems manage their own component data.
        std::unordered_map<Entity, TransformComponent> transforms_;
        std::unordered_map<Entity, RigidBodyComponent> rigidBodies_;
    };

} // namespace Framework
