#include "PhysicsSystem.h"
#include <algorithm>

namespace Framework {

    PhysicsSystem::PhysicsSystem(EntityStorage& storage) : ecs_(&storage) {}

    void PhysicsSystem::Update(float dt) {

        // If paused, skip all physics calculations for this frame.
        if (isPaused_) {
            return;
        }

        const float timeStep = 1.0f / 60.0f;
        timeAccumulator_ += dt;

        // Use a fixed time step for stability
        while (timeAccumulator_ >= timeStep) {
            Step(timeStep);
            timeAccumulator_ -= timeStep;
        }
    }

    void PhysicsSystem::Step(float dt) {

        // Before any calculations, assume the player is not grounded.
        // It will be set to true again in ResolveCollisions if a ground contact is found.
        for (auto& [entity, state] : ecs_->playerState) {
            state.isGrounded = false;
        }


        // --- 1. Apply Forces ---
        for (auto const& [entity, body] : ecs_->rigidBody) {
            if (body.InverseMass > 0.0f) {
                ecs_->rigidBody[entity].Force += gravity_ * body.Mass;
            }
        }

        // --- 2. Integrate ---
        Integrate(dt);

        // --- 3. Detect Collisions ---
        DetectCollisions(dt);

        // --- 4. Resolve Collisions (Iterative for stability) ---
        for (int i = 0; i < 5; ++i) { // Iterative solver
            ResolveCollisions(dt);
        }

        // --- 5. Clear forces for the next frame ---
        for (auto& pair : ecs_->rigidBody) {
            pair.second.Force = Vector2D::Zero();
        }
    }

    void PhysicsSystem::Integrate(float dt) {
        // Semi-implicit Euler integration
        for (auto const& [entity, body] : ecs_->rigidBody) {
            if (ecs_->velocity.count(entity) && ecs_->position.count(entity)) {
                auto& vel = ecs_->velocity.at(entity);
                auto& pos = ecs_->position.at(entity);

                // Update velocity: v += (F/m) * dt
                vel.Vel += (body.Force * body.InverseMass) * dt;

                // Update position: p += v * dt
                pos.Pos += vel.Vel * dt;
            }
        }
    }

    void PhysicsSystem::DetectCollisions(float dt) {
        contacts_.clear();
        auto& colliders = ecs_->colliders;

        for (auto it_a = colliders.begin(); it_a != colliders.end(); ++it_a) {
            for (auto it_b = std::next(it_a); it_b != colliders.end(); ++it_b) {
                Entity e1 = it_a->first;
                Entity e2 = it_b->first;

                if (ecs_->rigidBody.at(e1).InverseMass == 0.0f && ecs_->rigidBody.at(e2).InverseMass == 0.0f)
                    continue;

                AABB a = { ecs_->position.at(e1).Pos, it_a->second.HalfSize };
                AABB b = { ecs_->position.at(e2).Pos, it_b->second.HalfSize };

                Vector2D vel_a = ecs_->velocity.at(e1).Vel;
                Vector2D vel_b = ecs_->velocity.at(e2).Vel;

                Contact contact;
                // CHANGED: Use the new, more robust SweptAABB test
                if (SweptAABB(a, vel_a, b, vel_b, dt, contact)) {
                    contact.A = e1;
                    contact.B = e2;
                    contacts_.push_back(contact);
                }
            }
        }
    }

    void PhysicsSystem::ResolveCollisions(float dt) {
        for (const auto& contact : contacts_) {
            auto& posA = ecs_->position.at(contact.A);
            auto& posB = ecs_->position.at(contact.B);
            auto& rbA = ecs_->rigidBody.at(contact.A);
            auto& rbB = ecs_->rigidBody.at(contact.B);

            // --- Positional Correction ---
            const float percent = 0.4f;
            const float slop = 0.01f;
            Vector2D correction = contact.Normal * (std::max(contact.Penetration - slop, 0.0f) / (rbA.InverseMass + rbB.InverseMass)) * percent;
            posA.Pos -= correction * rbA.InverseMass;
            posB.Pos += correction * rbB.InverseMass;

            // --- Impulse Resolution ---
            auto& velA = ecs_->velocity.at(contact.A);
            auto& velB = ecs_->velocity.at(contact.B);
            Vector2D rv = velB.Vel - velA.Vel;
            float velAlongNormal = rv.Dot(contact.Normal);
            if (velAlongNormal > 0) continue;

            float e = 0.1f;
            float j = -(1 + e) * velAlongNormal;
            j /= rbA.InverseMass + rbB.InverseMass;
            Vector2D impulse = contact.Normal * j;
            velA.Vel -= impulse * rbA.InverseMass;
            velB.Vel += impulse * rbB.InverseMass;

            // --- Update Player Grounded State ---
            // If entity A is a player and it's colliding with something below it, it's grounded.
            if (ecs_->playerState.count(contact.A) && contact.Normal.y > 0.7f) {
                ecs_->playerState.at(contact.A).isGrounded = true;
            }
            // If entity B is a player and it's colliding with something above it (normal points away from B), it's grounded.
            if (ecs_->playerState.count(contact.B) && contact.Normal.y < -0.7f) {
                ecs_->playerState.at(contact.B).isGrounded = true;
            }
        }
    }
} // namespace Framework