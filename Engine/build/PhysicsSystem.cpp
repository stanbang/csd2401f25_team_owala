
#include "PhysicsSystem.h"

namespace Framework {

    void PhysicsSystem::Update(float dt) {
        // This is the core physics loop.

        // 1. Apply forces (like gravity)
        for (auto& pair : rigidBodies_) {
            RigidBodyComponent& body = pair.second;
            if (body.inverseMass > 0.0f) { // Don't apply gravity to static objects
                // Apply gravity: F = m * g
                body.force += gravity * body.mass;
            }
        }

        // 2. Integrate (update velocity and position)
        for (auto const& [entity, body] : rigidBodies_) {
            // This is explicit Euler integration. It's simple but can be unstable
            // with large timesteps. For this game, it's a great starting point.

            // Get mutable references to the components we need to update
            auto& mutableBody = rigidBodies_.at(entity);
            auto& transform = transforms_.at(entity);

            // a = F / m  (or a = F * inv_m for efficiency)
            mutableBody.acceleration = mutableBody.force * mutableBody.inverseMass;

            // v = v + a * dt
            mutableBody.velocity += mutableBody.acceleration * dt;

            // p = p + v * dt
            transform.position += mutableBody.velocity * dt;
        }

        // 3. Clear all forces for the next frame
        for (auto& pair : rigidBodies_) {
            pair.second.force = { 0.0f, 0.0f };
        }
    }

    // --- Component Management Methods ---

    void PhysicsSystem::AddTransform(Entity entity, Vector2D position) {
        transforms_[entity] = TransformComponent{ position };
    }

    void PhysicsSystem::AddRigidBody(Entity entity, bool isStatic) {
        rigidBodies_[entity] = RigidBodyComponent{};
        if (isStatic) {
            rigidBodies_[entity].SetMass(0.0f); // Infinite mass
        }
        else {
            rigidBodies_[entity].SetMass(1.0f);
        }
    }

    TransformComponent& PhysicsSystem::GetTransform(Entity entity) {
        return transforms_.at(entity);
    }

    RigidBodyComponent& PhysicsSystem::GetRigidBody(Entity entity) {
        return rigidBodies_.at(entity);
    }

} // namespace Framework

