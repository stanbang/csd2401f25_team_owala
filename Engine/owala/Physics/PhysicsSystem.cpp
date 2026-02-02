/*****************************************************************//**
* \file   PhysicsSystem.cpp
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief Implements the PhysicsSystem's fixed Step loop, force-based 
* integration, broad-phase grid logic, and narrow-phase AABB collision resolution.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "PhysicsSystem.h"
#include "EntityManager.h"
#include "AABB_Collision.h"
#include "FileUtility.h"
#include "OwalaTime.h"
#include "GameManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {
    // ----- Layer Collision Functions ------- //

     // Constants for edge handling for level editor
    static constexpr float EDGE_TOLERANCE = 1.0f;      // width tolerance
    static constexpr float VELOCITY_THRESHOLD = 5.0f;  // Min velocity to count as moving into
    static constexpr float EDGE_BIAS = 3.0f;           // Prefer vertical resolution on edges

    void PhysicsSystem::InitializeCollisionMatrix() {
        // Resize the matrix to fit all layers
        collisionMatrix_.resize(static_cast<size_t>(CollisionLayer::Count));
        // By default, make all layers collide with all other layers
        for (auto& row : collisionMatrix_) {
            row.set(); // Sets all bits to true
        }
    }

    bool PhysicsSystem::GetLayerCollision(CollisionLayer layer1, CollisionLayer layer2) const {
        size_t l1 = static_cast<size_t>(layer1);
        size_t l2 = static_cast<size_t>(layer2);

        if (l1 < collisionMatrix_.size() && l2 < collisionMatrix_.size()) {
            return collisionMatrix_[l1][l2];
        }
        return false;
    }

    void PhysicsSystem::SetLayerCollision(CollisionLayer layer1, CollisionLayer layer2, bool canCollide) {
        size_t l1 = static_cast<size_t>(layer1);
        size_t l2 = static_cast<size_t>(layer2);

        if (l1 < collisionMatrix_.size() && l2 < collisionMatrix_.size()) {
            // Set the interaction symmetrically
            collisionMatrix_[l1][l2] = canCollide;
            collisionMatrix_[l2][l1] = canCollide;
        }
    }



    void PhysicsSystem::Initialize() {
        InitializeCollisionMatrix();
        FileIOSystem::LoadWorldConfig(gravity_.y);
        // --- Initialize the Implicit Grid ---
        grid_.resize(GridCellsX * GridCellsY);
        // reserve space for performance
        colliderEntities_.reserve(maxCollidableEntities);
        entityToColliderIndex_.reserve(maxCollidableEntities);
    }

    // ----------------------------------------------------------------------------
    // The main update loop, which uses a fixed timestep for stability.
    // This accumulates delta time and runs the simulation
    // ----------------------------------------------------------------------------
    void PhysicsSystem::Update() {
        static bool debugOnce = true;
        if (debugOnce) {
            debugOnce = false;
            Logger::Get().Info(DebugType::PhysicsSystem)
                << "Player-Platform collision enabled: "
                << GetLayerCollision(CollisionLayer::Player, CollisionLayer::Platform)
                << std::endl;
        }

        if (isPaused_ || stepModeActive_) {
            return; // If paused or in step mode, do not automatically advance time.
        }

		int steps = Framework::Time::ConsumeFixedSteps();

        for (int i = 0; i < steps; ++i) {
            Step(Framework::Time::FixedDeltaTime());
		}
    }

    // ----------------------------------------------------------------------------
    // A single step of the physics simulation pipeline. This is the main
    // sequence of physics operations for one fixed timestep.
    // ----------------------------------------------------------------------------
    void PhysicsSystem::Step(float dt) {
        contacts_.clear();

		EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Reset player state (assume not grounded until a collision)
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerStateType)) {
            // Check if Player is active in the scene. Updates state if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
            state.isGrounded = false;
        }

        // Apply forces (only gravity for now)
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, RigidBodyType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

			RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);

            if (body.InverseMass > 0.0f && body.isAffectedByGravity) { 
                body.Force += gravity_ * body.Mass;
            }
        }

        //  Update position and velocity based on forces
        Integrate(dt);

        // Ensure Player is not able to move out of viewport bounds
        //EnforceViewportBounds();

        // Detect and resolve collisions
        ResolveCollisions(dt);

        // Clear all forces for the next frame
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, RigidBodyType)) {
            RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);
            body.Force = Vector2D::Zero();
        }
    }

    // ----------------------------------------------------------------------------
    // Updates position and velocity using Semi-Implicit Euler integration.
    // ----------------------------------------------------------------------------
    void PhysicsSystem::Integrate(float dt) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, RigidBodyType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

            if (entityMgr.HasComponent(entity, TransformType) && entityMgr.HasComponent(entity, VelocityType)) {
				RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);
				//Position& pos = entityMgr.GetComponent<Position>(entity);
				Velocity& vel = entityMgr.GetComponent<Velocity>(entity);

                vel.Vel += (body.Force * body.InverseMass) * dt;
            }
        }

        //Update positions of everything
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(true, TransformType, VelocityType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

			Transform& transform = entityMgr.GetComponent<Transform>(entity);
			Velocity& vel = entityMgr.GetComponent<Velocity>(entity);

			transform.position += vel.Vel *  dt;
        }
    }

    // ----------------------------------------------------------------------------
    // Implementation of the function to keep the player on-screen.
    // It clamps the player's position to the viewport boundaries (-1.0 to 1.0).
   // ----------------------------------------------------------------------------
    void PhysicsSystem::EnforceViewportBounds() {
        // The viewport uses  (NDC), from -1.0 to 1.0
        const float minX = -1.0f;
        const float maxX = 1.0f;
        const float minY = -1.0f;
        const float maxY = 1.0f;

        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        std::vector<Entity> constrainedEntities;

        auto players = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
        constrainedEntities.insert(constrainedEntities.end(), players.begin(), players.end());

        auto trapPlatforms = entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType);
        constrainedEntities.insert(constrainedEntities.end(), trapPlatforms.begin(), trapPlatforms.end());

        for (const Entity& entity : constrainedEntities) {
            if (entityMgr.HasComponent(entity, TransformType) && entityMgr.HasComponent(entity, VelocityType) && entityMgr.HasComponent(entity, AABBColliderType)) {

                Vector2D& pos = entityMgr.GetComponent<Transform>(entity).position;
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
        float snappedRotation = std::round(rotationDegrees / 90.0f) * 90.0f;
        snappedRotation = std::fmod(snappedRotation + 360.0f, 360.0f);
        bool swapDimensions = (std::abs(snappedRotation - 90.0f) < 1.0f) || (std::abs(snappedRotation - 270.0f) < 1.0f);
        Vector2D newHalfSize = halfSize;
        if (swapDimensions) {
            newHalfSize = { halfSize.y, halfSize.x };
        }
        return {
            {center.x - newHalfSize.x, center.y - newHalfSize.y},
            {center.x + newHalfSize.x, center.y + newHalfSize.y}
        };
    }

    bool PhysicsSystem::CheckTriggerOverlap(Entity e1, Entity e2) {

        GameManager& gm = GameManager::GetInstance();

     
        // We use the pointer *gm.GetSystems().ecs_ to get the reference
        if (!gm.GetSystems().ecs_) return false; // Safety check
        EntityManager entityMgr(*gm.GetSystems().ecs_);

        // check for correct entity
        if (!entityMgr.ValidateEntity(e1) || !entityMgr.ValidateEntity(e2)) return false;

        //Safety Check for ONLY pos and AABB , not RIGIDBODY
        if (!entityMgr.HasComponent(e1, Framework::TransformType) ||
            !entityMgr.HasComponent(e1, Framework::AABBColliderType)) return false;
        if (!entityMgr.HasComponent(e2, Framework::TransformType) ||
            !entityMgr.HasComponent(e2, Framework::AABBColliderType)) return false;

        // entity 1 data
        const Vector2D& pos1 = entityMgr.GetComponent<Framework::Transform>(e1).position;
        const Vector2D& halfSize1 = entityMgr.GetComponent<Framework::AABBCollider>(e1).HalfSize;
        const AABBCollider& collider1 = entityMgr.GetComponent<Framework::AABBCollider>(e1);

        Vector2D colliderPos1 = pos1 + collider1.Offset;
        float rot1 = 0.0f;
        if (entityMgr.HasComponent(e1, Framework::RotationFlagType)) {
            rot1 = entityMgr.GetComponent<Framework::RotationFlag>(e1).currentRotation;
        }

        // entity 2 data
        const Vector2D& pos2 = entityMgr.GetComponent<Framework::Transform>(e2).position;
        const Vector2D& halfSize2 = entityMgr.GetComponent<Framework::AABBCollider>(e2).HalfSize;
        const AABBCollider& collider2 = entityMgr.GetComponent<Framework::AABBCollider>(e2);
        float rot2 = 0.0f;
        Vector2D colliderPos2 = pos2 + collider2.Offset;
        if (entityMgr.HasComponent(e2, Framework::RotationFlagType)) {
            rot2 = entityMgr.GetComponent<Framework::RotationFlag>(e2).currentRotation;
        }

        //Calculate Bounds 
        Framework::AABB_Bounds bounds1 = PhysicsSystem::GetRotatedAABB(colliderPos1, halfSize1, rot1);
        Framework::AABB_Bounds bounds2 = PhysicsSystem::GetRotatedAABB(colliderPos2, halfSize2, rot2);

        
        bool collisionX = bounds1.max.x >= bounds2.min.x && bounds2.max.x >= bounds1.min.x;
        bool collisionY = bounds1.max.y >= bounds2.min.y && bounds2.max.y >= bounds1.min.y;

        return collisionX && collisionY;
    }

    // ----------------------------------------------------------------------------
    // Detects and resolves collisions using direct positional correction.
    // ----------------------------------------------------------------------------
    void PhysicsSystem::ResolveCollisions(float dt) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
        std::vector<Entity> allColliders = entityMgr.GetAllEntitiesWithComponents(true, AABBColliderType, LoadedTagType, TransformType);

        // --- Check Entity Count OR DEBUG TOGGLE ---
        if (forceNarrowPhase_ || allColliders.size() > maxCollidableEntities) {
            if (!forceNarrowPhase_ && allColliders.size() > maxCollidableEntities) { // preventive measure for too much entities
                // Log an error and fall back to narrow based implementation to prevent crashing
                static bool errorLogged = false;
                if (!errorLogged) {
                    Logger::Get().Error(DebugType::PhysicsSystem)
                        << "More than " << maxCollidableEntities << " colliders! ";
                    errorLogged = true;
                }
            }

            // Fallback to narrow based implementation
            for (std::vector<Entity>::iterator it_a = allColliders.begin(); it_a != allColliders.end(); ++it_a) {
                for (std::vector<Entity>::iterator it_b = std::next(it_a); it_b != allColliders.end(); ++it_b) {
                    CheckCollisionPair(*it_a, *it_b, dt);
                }
            }
            return;
        }
    

        // ---  BUILD MAPPINGS ---
        // Create the two-way map between Entity ID and bitset index
        entityToColliderIndex_.clear();
        colliderEntities_.clear(); //  reserved in Initialize

        for (int i = 0; i < allColliders.size(); ++i) {
            Entity entity = allColliders[i];
            entityToColliderIndex_[entity] = i;
            colliderEntities_.push_back(entity);
        }

        // --- Insert all entities into grid ---
        // Clear all bits in the grid from the last frame
        for (std::vector<std::bitset<maxCollidableEntities>>::iterator::value_type& cell : grid_) {
            cell.reset();
        }
        for (int i = 0; i < colliderEntities_.size(); ++i) {
            Entity entity = colliderEntities_[i];

            // Get the AABB_Bounds for insertion
            const Vector2D& pos = entityMgr.GetComponent<Transform>(entity).position;
            const Vector2D& halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
            Vector2D colliderPos = pos + collider.Offset;

            float colliderRotation = 0.0f;
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                colliderRotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
            }
            if (entityMgr.HasComponent(entity, TransformType)) {
                colliderRotation += entityMgr.GetComponent<Transform>(entity).rotation.colliderAngle * (180.0f / static_cast<float>(M_PI));
            }

            AABB_Bounds bounds = GetRotatedAABB(colliderPos, halfSize, colliderRotation);

            GridInsert(i, bounds); // Insert the bitset index 'i'
        }

        // ---  QUERY PHASE (Check for collisions) ---
        // prevents checking the same pair twice (e.g., A-B and B-A)
        std::set<std::pair<Entity, Entity>> checkedPairs;
        std::bitset<maxCollidableEntities> potentialPartners;

        for (int i = 0; i < colliderEntities_.size(); ++i) {
            Entity e1 = colliderEntities_[i];

            // Get the AABB for e1 to query the grid
            const Vector2D& pos1 = entityMgr.GetComponent<Transform>(e1).position;
            const Vector2D& halfSize1 = entityMgr.GetComponent<AABBCollider>(e1).HalfSize;
            float rotation1 = 0.0f;
            if (entityMgr.HasComponent(e1, RotationFlagType)) {
                rotation1 = entityMgr.GetComponent<RotationFlag>(e1).currentRotation;
            }
            AABB_Bounds bounds1 = GetRotatedAABB(pos1, halfSize1, rotation1);

            // Get all partners from the grid as bitset
            GridQuery(bounds1, potentialPartners);

            // potentialPartners has a '1' for the index of
            // every entity that could be colliding with e1

            for (int j = 0; j < colliderEntities_.size(); ++j) {
                if (i == j) continue; // Don't check against self

                if (potentialPartners.test(j)) {
                    Entity e2 = colliderEntities_[j];

                    // check only each pair once
                    Entity first = std::min(e1, e2);
                    Entity second = std::max(e1, e2);


                    if (checkedPairs.insert({ first, second }).second) {
                        // This is a new pair, use the narrow-phase check
                        CheckCollisionPair(first, second, dt);
                    }
                }
            }
        }
    }


    // ----------------------------------------------------------------------------
    // --- NARROW-PHASE implementation ---
    // Previous Implementation of resolve collisions
    // ----------------------------------------------------------------------------
    void PhysicsSystem::CheckCollisionPair(Entity e1, Entity e2, float dt) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Check entities have RigidBody component
        if (!entityMgr.HasComponent(e1, RigidBodyType) || !entityMgr.HasComponent(e2, RigidBodyType)) {
            //Logger::Get().Error(DebugType::PhysicsSystem) << "Entities are missing RigidBody component. Unable to CheckCollisionPair" << std::endl;
            return;
        }
        // Check entities have Position component
        if (!entityMgr.HasComponent(e1, TransformType) || !entityMgr.HasComponent(e2, TransformType)) {
            Logger::Get().Error(DebugType::PhysicsSystem) << "Entities are missing Transform component. Unable to CheckCollisionPair" << std::endl;
            return;
        }
        // Check entities have AABBCollider component
        if (!entityMgr.HasComponent(e1, AABBColliderType) || !entityMgr.HasComponent(e2, AABBColliderType)) {
            Logger::Get().Error(DebugType::PhysicsSystem) << "Entities are missing AABBCollider component. Unable to CheckCollisionPair" << std::endl;
            return;
        }


        // --- LAYER-BASED COLLISION CHECK ---
        // If either entities is default layer, skip them.
        if (entityMgr.GetProperties(e1).layer == CollisionLayer::Default || entityMgr.GetProperties(e2).layer == CollisionLayer::Default) {
            return;
        }

        CollisionLayer& layer1 = entityMgr.GetProperties(e1).layer;
        CollisionLayer& layer2 = entityMgr.GetProperties(e2).layer;

        // Check the collision matrix. If they can't collide, skip.
        if (!collisionMatrix_[static_cast<size_t>(layer1)][static_cast<size_t>(layer2)]) {
            return;
        }

        if (entityMgr.GetComponent<RigidBody>(e1).InverseMass == 0.0f && entityMgr.GetComponent<RigidBody>(e2).InverseMass == 0.0f) {
            return; // Skip static-vs-static
        }

        // --- Get rotation for e1 ---
        float rotation1 = 0.0f;
        if (entityMgr.HasComponent(e1, RotationFlagType)) {
            rotation1 = entityMgr.GetComponent<RotationFlag>(e1).currentRotation;
        }
        if (entityMgr.HasComponent(e1, TransformType)) {
            rotation1 += entityMgr.GetComponent<Transform>(e1).rotation.colliderAngle * (180.0f / static_cast<float>(M_PI));
        }

        // --- Get rotation for e2 ---
        float rotation2 = 0.0f;
        if (entityMgr.HasComponent(e2, RotationFlagType)) {
            rotation2 = entityMgr.GetComponent<RotationFlag>(e2).currentRotation;
        }
        if (entityMgr.HasComponent(e2, TransformType)) {
            rotation2 += entityMgr.GetComponent<Transform>(e2).rotation.colliderAngle * (180.0f / static_cast<float>(M_PI));
        }


        // --- Convert to min/max format for the collision functions ---
        const Vector2D& pos1 = entityMgr.GetComponent<Transform>(e1).position;
        const AABBCollider& collider1 = entityMgr.GetComponent<AABBCollider>(e1);
        Vector2D rotatedOffset1 = collider1.Offset;
        if (std::abs(rotation1) > 0.01f) {
            float rotRad = rotation1 * (static_cast<float>(M_PI) / 180.0f);
            Matrix3x3 rotMat = Matrix3x3::CreateRotation(rotRad);
            rotatedOffset1 = rotMat * collider1.Offset;
        }
        Vector2D colliderPos1 = pos1 + rotatedOffset1;
        AABB_Bounds bounds1 = GetRotatedAABB(colliderPos1, collider1.HalfSize, rotation1);

        const Vector2D& pos2 = entityMgr.GetComponent<Transform>(e2).position;
        const AABBCollider& collider2 = entityMgr.GetComponent<AABBCollider>(e2);
        Vector2D rotatedOffset2 = collider2.Offset;
        if (std::abs(rotation2) > 0.01f) {
            float rotRad = rotation2 * (static_cast<float>(M_PI) / 180.0f);
            Matrix3x3 rotMat = Matrix3x3::CreateRotation(rotRad);
            rotatedOffset2 = rotMat * collider2.Offset;
        }
        Vector2D colliderPos2 = pos2 + rotatedOffset2;
        AABB_Bounds bounds2 = GetRotatedAABB(colliderPos2, collider2.HalfSize, rotation2);



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

        // --- Perform Resolution if overlapping ---
        if (contact.State == CollisionState::Overlapping) {
            // Ensure e1 is always the dynamic object for consistent resolution
            Entity dynE = e1, staticE = e2;
            if (entityMgr.GetComponent<RigidBody>(dynE).InverseMass == 0.0f) {
                std::swap(dynE, staticE); // swaps if E1 is the static object IE. Mass = 0
            }
            // Check dynE has Velocity component
            if (!entityMgr.HasComponent(dynE, VelocityType)) {
                Logger::Get().Error(DebugType::PhysicsSystem) << "dynE entity missing Velocity component. Unable to CheckCollisionPair" << std::endl;
                return;
            }

            Vector2D& dynPos = entityMgr.GetComponent<Transform>(dynE).position;
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

            Vector2D& dynVel = entityMgr.GetComponent<Velocity>(dynE).Vel;

            // Calculate biased overlaps (favor vertical for platformers)
            float biasedMinOverlapY = minOverlapY - EDGE_BIAS;

            if (biasedMinOverlapY < minOverlapX) {
                // VERTICAL COLLISION - priority for platformers
                if (overlapTop < overlapBottom) {
                    // Hit ceiling
                    dynPos.y -= (overlapTop + EDGE_TOLERANCE);
                    if (dynVel.y > 0) dynVel.y = 0;
                }
                else {
                    // Landed on platform
                    dynPos.y += (overlapBottom + EDGE_TOLERANCE);
                    if (dynVel.y < 0) dynVel.y = 0;

                    if (entityMgr.HasComponent(dynE, PlayerStateType)) {
                        entityMgr.GetComponent<PlayerState>(dynE).isGrounded = true;
                    }

                    // Trap platform handling
                    if (entityMgr.HasComponent(dynE, TrapPlatformComponentType)) {
                        TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(dynE);
                        if (trap.hasFallen && trap.airTime > 0.1f) {
                            Vector2D pos = entityMgr.GetComponent<Transform>(dynE).position;
                            MessageBus::Instance().Publish(TrapPlatformLandEvent(pos));
                        }
                        trap.hasFallen = false;
                        trap.isGrounded = true;
                    }
                }
            }
            else {
                // HORIZONTAL COLLISION - only resolve if actively moving into the edge
                bool movingLeft = dynVel.x < -VELOCITY_THRESHOLD;
                bool movingRight = dynVel.x > VELOCITY_THRESHOLD;

                if (overlapLeft < overlapRight) {
                    // Would push left - only if moving right into it
                    if (movingRight || std::abs(dynVel.x) < 0.1f) {
                        dynPos.x -= (overlapLeft + EDGE_TOLERANCE);
                        if (dynVel.x > 0) dynVel.x = 0;
                    }
                }
                else {
                    // Would push right - only if moving left into it
                    if (movingLeft || std::abs(dynVel.x) < 0.1f) {
                        dynPos.x += (overlapRight + EDGE_TOLERANCE);
                        if (dynVel.x < 0) dynVel.x = 0;
                    }
                }
            }

            contacts_.push_back(contact);

            //    if (minOverlapY < minOverlapX) {
            //        // Vertical collision has priority for platformers
            //        if (overlapTop < overlapBottom) {
            //            dynPos.y -= overlapTop; // Push down (hit head)
            //            if (dynVel.y > 0) dynVel.y = 0;
            //        }
            //        else {
            //            dynPos.y += overlapBottom; // Push up (landed)
            //            if (dynVel.y < 0) dynVel.y = 0;
            //            if (entityMgr.HasComponent(dynE, PlayerStateType)) entityMgr.GetComponent<PlayerState>(dynE).isGrounded = true;
            //            // Check if the entity landing is a Trap Platform
            //            if (entityMgr.HasComponent(dynE, TrapPlatformComponentType)) {
            //                TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(dynE);

            //                // Only trigger if it was actually in 'falling mode'
            //                if (trap.hasFallen) {
            //                    // for edge case where platforms are in a corner and "falls" for only 1 frame
            //                    if (trap.airTime > 0.1f) {
            //                        Vector2D pos = entityMgr.GetComponent<Transform>(dynE).position;
            //                        MessageBus::Instance().Publish(TrapPlatformLandEvent(pos));
            //                    }

            //                    trap.hasFallen = false;
            //                    trap.isGrounded = true;
            //                }
            //            }
            //        }
            //    }
            //    else {
            //        // Horizontal collision
            //        if (overlapLeft < overlapRight) {
            //            dynPos.x -= overlapLeft; // Push left
            //        }
            //        else {
            //            dynPos.x += overlapRight; // Push right
            //        }
            //    }
            //}
           // contacts_.push_back(contact); // Log the contact regardless of resolution
        }
    }

    // ------ GRID HELPER FUNCTIONS ------ //

    int PhysicsSystem::GetCellCoordX(float worldPos) const {
        int coord = static_cast<int>(std::floor((worldPos - gridWorldMinX) * invCellSize));
        return std::clamp(coord, 0, GridCellsX - 1);
    }

    
    int PhysicsSystem::GetCellCoordY(float worldPos) const {
        int coord = static_cast<int>(std::floor((worldPos - gridWorldMinY) * invCellSize));
        return std::clamp(coord, 0, GridCellsY - 1);
    }

   
    int PhysicsSystem::GetCellIndex(int x, int y) const {
        return x + y * GridCellsX;
    }

    
    void PhysicsSystem::GridInsert(int colliderIndex, const AABB_Bounds& bounds) {
        int minX = GetCellCoordX(bounds.min.x);
        int minY = GetCellCoordY(bounds.min.y);
        int maxX = GetCellCoordX(bounds.max.x);
        int maxY = GetCellCoordY(bounds.max.y);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                grid_[GetCellIndex(x, y)].set(colliderIndex);
            }
        }
    }

    
    void PhysicsSystem::GridQuery(const AABB_Bounds& bounds, std::bitset<maxCollidableEntities>& potentialColliders) const {
        int minX = GetCellCoordX(bounds.min.x);
        int minY = GetCellCoordY(bounds.min.y);
        int maxX = GetCellCoordX(bounds.max.x);
        int maxY = GetCellCoordY(bounds.max.y);

        potentialColliders.reset(); // Clear the set
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                potentialColliders |= grid_[GetCellIndex(x, y)];
            }
        }
    }

    //--------- DEBUG DRAW FUNCTIONS ---------//
    std::vector<PhysicsDebugCell> PhysicsSystem::GetDebugDrawData(Framework::CameraSystem* cameraSystem, Entity cameraID) {
        std::vector<PhysicsDebugCell> data;
        if (!drawDebugGrid || !cameraSystem || cameraID == 0) {
            return data;
        }

        // Get view bounds from the camera
        float viewLeft, viewRight, viewBottom, viewTop;
        cameraSystem->GetViewBounds(cameraID, viewLeft, viewRight, viewBottom, viewTop);

        // Find the grid cell range that is visible
        int minX = GetCellCoordX(viewLeft) - 1;
        int maxX = GetCellCoordX(viewRight) + 1;
        int minY = GetCellCoordY(viewBottom) - 1;
        int maxY = GetCellCoordY(viewTop) + 1;

        // Clamp to the grid's actual boundaries
        minX = std::max(0, minX);
        maxX = std::min(GridCellsX - 1, maxX);
        minY = std::max(0, minY);
        maxY = std::min(GridCellsY - 1, maxY);

        // Reserve space for efficiency
        data.reserve(static_cast<size_t>(maxX - minX + 1) * (maxY - minY + 1));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                PhysicsDebugCell cell;
                // Get the world-space corners of this cell
                cell.worldMin = {
                    gridWorldMinX + x * cellSize,
                    gridWorldMinY + y * cellSize
                };
                cell.worldMax = {
                    cell.worldMin.x + cellSize,
                    cell.worldMin.y + cellSize
                };
                // Check if this cell has any objects in it
                cell.isActive = grid_[GetCellIndex(x, y)].any();
                data.push_back(cell);
            }
        }
        return data;
    }
   

} // namespace Framework