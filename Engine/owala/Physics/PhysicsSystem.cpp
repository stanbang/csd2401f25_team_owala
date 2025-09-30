// PhysicsSystem.cpp
#include "PhysicsSystem.h"
#include "EntityManager.h"
#include "AABB_Collision.h"
#include <algorithm>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

		EntityManager entityMgr(*ecs_);

        // 1. Reset player state (assume not grounded until a collision proves otherwise).
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
            // Check if Player is active in the scene. Updates state if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
            state.isGrounded = false;
        }

        // 2. Apply forces (only gravity for now).
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RigidBodyType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

			RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);

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
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RigidBodyType)) {
            RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);
            body.Force = Vector2D::Zero();
        }
    }

    // Updates position and velocity using Semi-Implicit Euler integration.
    void PhysicsSystem::Integrate(float dt) {
		EntityManager entityMgr(*ecs_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RigidBodyType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            if (entityMgr.HasComponent(entity, PositionType) && entityMgr.HasComponent(entity, VelocityType)) {
				RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);
				Position& pos = entityMgr.GetComponent<Position>(entity);
				Velocity& vel = entityMgr.GetComponent<Velocity>(entity);

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
		EntityManager entityMgr(*ecs_);
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (entityMgr.HasComponent(entity, PositionType) && entityMgr.HasComponent(entity, AABBColliderType)) {
				Vector2D& pos = entityMgr.GetComponent<Position>(entity).Pos;
				Vector2D& vel = entityMgr.GetComponent<Velocity>(entity).Vel;

                const Vector2D& halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;

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

    AABB_Bounds PhysicsSystem::GetRotatedAABB(const Vector2D& center, const Vector2D& halfSize, float rotationDegrees) {
        if (std::abs(rotationDegrees) < 0.001f) {
            return {
                {center.x - halfSize.x, center.y - halfSize.y},
                {center.x + halfSize.x, center.y + halfSize.y}
            };
        }

        float rotationRad = static_cast<float>(rotationDegrees * M_PI / 180.0f);
        float cos_r = std::cos(rotationRad);
        float sin_r = std::sin(rotationRad);

        Vector2D corners[4] = {
            {-halfSize.x, -halfSize.y},  
            { halfSize.x, -halfSize.y},  
            { halfSize.x,  halfSize.y},  
            {-halfSize.x,  halfSize.y}   
        };

        // Rotate each corner and find the new min/max bounds
        float minX = INFINITY, maxX = -INFINITY;
        float minY = INFINITY, maxY = -INFINITY;

        for (int i = 0; i < 4; i++) {
            // Rotate the corner
            float rotatedX = corners[i].x * cos_r - corners[i].y * sin_r;
            float rotatedY = corners[i].x * sin_r + corners[i].y * cos_r;

            // Translate to world position
            rotatedX += center.x;
            rotatedY += center.y;

            // Update bounds
            minX = std::min(minX, rotatedX);
            maxX = std::max(maxX, rotatedX);
            minY = std::min(minY, rotatedY);
            maxY = std::max(maxY, rotatedY);
        }

        return { {minX, minY}, {maxX, maxY} };
    }

    // Detects and resolves collisions using direct positional correction.
    void PhysicsSystem::ResolveCollisions(float dt) {
        EntityManager entityMgr(*ecs_);
        std::vector<Entity> colliders = entityMgr.GetAllEntitiesWithComponent(AABBColliderType);

        for (auto it_a = colliders.begin(); it_a != colliders.end(); ++it_a) {
            for (auto it_b = std::next(it_a); it_b != colliders.end(); ++it_b) {
                Entity e1 = *it_a;
                Entity e2 = *it_b;

                float invMass1 = entityMgr.GetComponent<RigidBody>(e1).InverseMass;
                float invMass2 = entityMgr.GetComponent<RigidBody>(e2).InverseMass;

                // Skip if both are static
                if (invMass1 == 0.0f && invMass2 == 0.0f)
                    continue;

                // --- Get rotation for each entity ---
                float rotation1 = 0.0f;
                if (entityMgr.HasComponent(e1, RotationFlagType)) {
                    rotation1 = entityMgr.GetComponent<RotationFlag>(e1).currentRotation;
                }

                float rotation2 = 0.0f;
                if (entityMgr.HasComponent(e2, RotationFlagType)) {
                    rotation2 = entityMgr.GetComponent<RotationFlag>(e2).currentRotation;
                }

                // --- Convert to min/max format for the collision functions ---
                const Vector2D& pos1 = entityMgr.GetComponent<Position>(e1).Pos;
                const Vector2D& halfSize1 = entityMgr.GetComponent<AABBCollider>(e1).HalfSize;
                AABB_Bounds bounds1 = GetRotatedAABB(pos1, halfSize1, rotation1);

                const Vector2D& pos2 = entityMgr.GetComponent<Position>(e2).Pos;
                const Vector2D& halfSize2 = entityMgr.GetComponent<AABBCollider>(e2).HalfSize;
                AABB_Bounds bounds2 = GetRotatedAABB(pos2, halfSize2, rotation2);

                // --- Determine Collision State ---
                Contact contact = { e1, e2, CollisionState::NoCollision };
                if (CollisionIntersection_RectRect_Static(bounds1, bounds2)) {
                    contact.State = CollisionState::Overlapping;
                }
                else {
                    float t;
                    if (CollisionIntersection_RectRect_Dynamic(bounds1, entityMgr.GetComponent<Velocity>(e1).Vel, bounds2, entityMgr.GetComponent<Velocity>(e2).Vel, dt, t)) {
                        contact.State = CollisionState::WillCollide;
                    }
                }

                // --- Perform Resolution only if overlapping ---
                if (contact.State == CollisionState::Overlapping) {
                    // Calculate overlaps on all four sides
                    float overlapLeft = bounds1.max.x - bounds2.min.x;
                    float overlapRight = bounds2.max.x - bounds1.min.x;
                    float overlapTop = bounds1.max.y - bounds2.min.y;
                    float overlapBottom = bounds2.max.y - bounds1.min.y;

                    // Find the minimum overlap
                    float minOverlapX = std::min(overlapLeft, overlapRight);
                    float minOverlapY = std::min(overlapTop, overlapBottom);

                    Vector2D& pos1 = entityMgr.GetComponent<Position>(e1).Pos;
                    Vector2D& pos2 = entityMgr.GetComponent<Position>(e2).Pos;
                    Vector2D& vel1 = entityMgr.GetComponent<Velocity>(e1).Vel;
                    Vector2D& vel2 = entityMgr.GetComponent<Velocity>(e2).Vel;

                    // Determine if both are dynamic or one is static
                    bool bothDynamic = (invMass1 > 0.0f && invMass2 > 0.0f);

                    if (minOverlapY < minOverlapX) {
                        // Vertical collision has priority for platformers
                        if (overlapTop < overlapBottom) {
                            // e1 hit its head on e2's bottom
                            if (bothDynamic) {
                                pos1.y -= overlapTop;
                                pos2.y += overlapTop;
                                if (vel1.y > 0) vel1.y = 0;
                                if (vel2.y < 0) vel2.y = 0;
                                if (entityMgr.HasComponent(e2, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e2).isGrounded = true;
                            }
                            else if (invMass1 > 0.0f) {
                                pos1.y -= overlapTop;
                                if (vel1.y > 0) vel1.y = 0;
                                if (entityMgr.HasComponent(e2, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e2).isGrounded = true;
                            }
                            else {
                                pos2.y += overlapTop;
                                if (vel2.y < 0) vel2.y = 0;
                                if (entityMgr.HasComponent(e2, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e2).isGrounded = true;
                            }
                        }
                        else {
                            // e1 landed on e2's top
                            if (bothDynamic) {               
                                pos1.y += overlapBottom;
                                pos2.y -= overlapBottom;
                                if (vel1.y < 0) vel1.y = 0;
                                if (vel2.y > 0) vel2.y = 0;
                                if (entityMgr.HasComponent(e1, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e1).isGrounded = true;
                            }
                            else if (invMass1 > 0.0f) {
                                pos1.y += overlapBottom;
                                if (vel1.y < 0) vel1.y = 0;
                                if (entityMgr.HasComponent(e1, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e1).isGrounded = true;
                            }
                            else {
                                pos2.y -= overlapBottom;
                                if (vel2.y > 0) vel2.y = 0;
                                if (entityMgr.HasComponent(e1, PlayerStateType)) entityMgr.GetComponent<PlayerState>(e1).isGrounded = true;
                            }
                        }
                    }
                    else {
                        // Horizontal collision
                        if (overlapLeft < overlapRight) {
                            // e1 collided from the right side
                            if (bothDynamic) {
                                pos1.x -= overlapLeft ;
                                pos2.x += overlapLeft;
                            }
                            else if (invMass1 > 0.0f) {
                                pos1.x -= overlapLeft;
                            }
                            else {
                                pos2.x += overlapLeft;
                            }
                        }
                        else {
                            // e1 collided from the left side
                            if (bothDynamic) {                              
                                pos1.x += overlapRight;
                                pos2.x -= overlapRight;
                            }
                            else if (invMass1 > 0.0f) {
                                pos1.x += overlapRight;
                            }
                            else {
                                pos2.x -= overlapRight;
                            }
                        }
                    }
                }
                contacts_.push_back(contact); // Log the contact regardless of resolution
            }
        }
    }

} // namespace Framework