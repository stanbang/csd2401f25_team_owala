// PhysicsSystem.cpp
#include "PhysicsSystem.h"
#include "AABB_Collision.h"
#include <algorithm>
#include <vector>

namespace Framework {

    PhysicsSystem::PhysicsSystem(EntityStorage& storage) : ecs_(&storage) {}

    // The main update loop, which uses a fixed timestep for physics stability.
    void PhysicsSystem::Update(float dt) {
        if (isPaused_ || stepModeActive_) {
            return; // If paused or in step mode, do not automatically advance time.
        }
        const float timeStep = 1.0f / 60.0f;
        timeAccumulator_ += dt;
        while (timeAccumulator_ >= timeStep) {
            Step(timeStep);
            timeAccumulator_ -= timeStep;
        }
    }

    // A single step of the physics simulation pipeline.
    void PhysicsSystem::Step(float dt) {
        contacts_.clear();

        // 1. Reset player state (assume not grounded until a collision proves otherwise).
        for (auto& [entity, state] : ecs_->playerState) {
            state.isGrounded = false;
        }

        // 2. Apply forces (only gravity for now).
        for (auto& [entity, body] : ecs_->rigidBody) {
            if (body.InverseMass > 0.0f) {
                body.Force += gravity_ * body.Mass;
            }
        }

        // 3. Update position and velocity based on forces.
        Integrate(dt);

        EnforceViewportBounds();

        // 4. Detect and resolve collisions.
        ResolveCollisions(dt);

        // 5. Clear all forces for the next frame.
        for (auto& [entity, body] : ecs_->rigidBody) {
            body.Force = Vector2D::Zero();
        }
    }

    // Updates position and velocity using Semi-Implicit Euler integration.
    void PhysicsSystem::Integrate(float dt) {
        for (auto& [entity, body] : ecs_->rigidBody) {
            if (ecs_->position.count(entity) && ecs_->velocity.count(entity)) {
                auto& pos = ecs_->position.at(entity);
                auto& vel = ecs_->velocity.at(entity);
                vel.Vel += (body.Force * body.InverseMass) * dt;
                pos.Pos += vel.Vel * dt;
            }
        }
    }

    // Implementation of the function to keep the player on-screen.
    void PhysicsSystem::EnforceViewportBounds() {
        // The viewport uses Normalized Device Coordinates (NDC), from -1.0 to 1.0.
        const float minX = -1.0f;
        const float maxX = 1.0f;
        const float minY = -1.0f;
        const float maxY = 1.0f;

        // Iterate through all entities with a player tag.
        for (const auto& [entity, tag] : ecs_->playerTag) {
            if (ecs_->position.count(entity) && ecs_->colliders.count(entity)) {
                auto& pos = ecs_->position.at(entity).Pos;
                auto& vel = ecs_->velocity.at(entity).Vel;
                const auto& halfSize = ecs_->colliders.at(entity).HalfSize;

                // Check and correct X-axis boundaries
                if (pos.x - halfSize.x < minX) {
                    pos.x = minX + halfSize.x;
                    if (vel.x < 0) vel.x = 0;
                }
                if (pos.x + halfSize.x > maxX) {
                    pos.x = maxX - halfSize.x;
                    if (vel.x > 0) vel.x = 0;
                }

                // Check and correct Y-axis boundaries
                if (pos.y - halfSize.y < minY) {
                    pos.y = minY + halfSize.y;
                    if (vel.y < 0) vel.y = 0;
                }
                if (pos.y + halfSize.y > maxY) {
                    pos.y = maxY - halfSize.y;
                    if (vel.y > 0) vel.y = 0;
                }
            }
        }
    }

    // Detects and resolves collisions using direct positional correction.
    void PhysicsSystem::ResolveCollisions(float dt) {
        auto& colliders = ecs_->colliders;

        for (auto it_a = colliders.begin(); it_a != colliders.end(); ++it_a) {
            for (auto it_b = std::next(it_a); it_b != colliders.end(); ++it_b) {
                Entity e1 = it_a->first;
                Entity e2 = it_b->first;

                if (ecs_->rigidBody.at(e1).InverseMass == 0.0f && ecs_->rigidBody.at(e2).InverseMass == 0.0f)
                    continue;

                // --- Convert to min/max format for the collision functions ---
                const auto& pos1 = ecs_->position.at(e1).Pos;
                const auto& halfSize1 = it_a->second.HalfSize;
                AABB_Bounds bounds1 = { {pos1.x - halfSize1.x, pos1.y - halfSize1.y}, {pos1.x + halfSize1.x, pos1.y + halfSize1.y} };

                const auto& pos2 = ecs_->position.at(e2).Pos;
                const auto& halfSize2 = it_b->second.HalfSize;
                AABB_Bounds bounds2 = { {pos2.x - halfSize2.x, pos2.y - halfSize2.y}, {pos2.x + halfSize2.x, pos2.y + halfSize2.y} };

                // --- Determine Collision State ---
                Contact contact = { e1, e2, CollisionState::NoCollision };
                if (CollisionIntersection_RectRect_Static(bounds1, bounds2)) {
                    contact.State = CollisionState::Overlapping;
                }
                else {
                    float t;
                    if (CollisionIntersection_RectRect_Dynamic(bounds1, ecs_->velocity.at(e1).Vel, bounds2, ecs_->velocity.at(e2).Vel, dt, t)) {
                        contact.State = CollisionState::WillCollide;
                    }
                }

                // --- Perform Resolution only if overlapping ---
                if (contact.State == CollisionState::Overlapping) {
                    // Ensure e1 is always the dynamic object for consistent resolution
                    Entity dynE = e1, staticE = e2;
                    if (ecs_->rigidBody.at(dynE).InverseMass == 0.0f) {
                        std::swap(dynE, staticE);
                    }

                    auto& dynPos = ecs_->position.at(dynE).Pos;
                    const auto& dynBounds = (dynE == e1) ? bounds1 : bounds2;
                    const auto& staticBounds = (dynE == e1) ? bounds2 : bounds1;

                    // Calculate overlaps on all four sides
                    float overlapLeft = dynBounds.max.x - staticBounds.min.x;
                    float overlapRight = staticBounds.max.x - dynBounds.min.x;
                    float overlapTop = dynBounds.max.y - staticBounds.min.y;
                    float overlapBottom = staticBounds.max.y - dynBounds.min.y;

                    // Find the minimum overlap
                    float minOverlapX = std::min(overlapLeft, overlapRight);
                    float minOverlapY = std::min(overlapTop, overlapBottom);

                    auto& dynVel = ecs_->velocity.at(dynE).Vel;

                    if (minOverlapY < minOverlapX) {
                        // Vertical collision has priority for platformers
                        if (overlapTop < overlapBottom) {
                            dynPos.y -= overlapTop; // Push down (hit head)
                            if (dynVel.y > 0) dynVel.y = 0;
                        }
                        else {
                            dynPos.y += overlapBottom; // Push up (landed)
                            if (dynVel.y < 0) dynVel.y = 0;
                            if (ecs_->playerState.count(dynE)) ecs_->playerState.at(dynE).isGrounded = true;
                        }
                    }
                    else {
                        // Horizontal collision
                        if (overlapLeft < overlapRight) {
                            dynPos.x -= overlapLeft; // Push left
                        }
                        else {
                            dynPos.x += overlapRight; // Push right
                        }
                    }
                }
                contacts_.push_back(contact); // Log the contact regardless of resolution
            }
        }
    }

} // namespace Framework