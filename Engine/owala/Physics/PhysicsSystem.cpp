/*****************************************************************//**
* \file   PhysicsSystem.cpp
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "PhysicsSystem.h"
#include "EntityManager.h"
#include "AABB_Collision.h"
#include "FileUtility.h"
#include <algorithm>
#include <vector>
#include <set> // Include for duplicate pair checking
#include "OwalaTime.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    PhysicsSystem::PhysicsSystem(EntityStorage& storage) : ecs_(&storage) {}

    void PhysicsSystem::InitializeCollisionMatrix() {
        // Resize the matrix to fit all layers
        collisionMatrix_.resize(static_cast<size_t>(CollisionLayer::Count));
        // By default, make all layers collide with all other layers
        for (auto& row : collisionMatrix_) {
            row.set(); // Sets all bits to true
        }
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
        grid_.resize(kGridCellsX * kGridCellsY);
        // Pre-allocate maps/vectors for performance
        colliderEntities_.reserve(kMaxCollidableEntities);
        entityToColliderIndex_.reserve(kMaxCollidableEntities);
    }

    // ----------------------------------------------------------------------------
    // The main update loop, which uses a fixed timestep for stability.
    // This accumulates delta time and runs the simulation
    // ----------------------------------------------------------------------------
    void PhysicsSystem::Update() {
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

		EntityManager entityMgr(*ecs_);

        // Reset player state (assume not grounded until a collision)
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
            // Check if Player is active in the scene. Updates state if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
            state.isGrounded = false;
        }

        // Apply forces (only gravity for now)
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RigidBodyType)) {
            // Check if Entity is active in the scene. Handle Physics if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

			RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);

            if (body.InverseMass > 0.0f) {
                body.Force += gravity_ * body.Mass;
            }
        }

        //  Update position and velocity based on forces
        Integrate(dt);

        // Ensure Player is not able to move out of viewport bounds
        EnforceViewportBounds();

        // Detect and resolve collisions
        ResolveCollisions(dt);

        // Clear all forces for the next frame
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RigidBodyType)) {
            RigidBody& body = entityMgr.GetComponent<RigidBody>(entity);
            body.Force = Vector2D::Zero();
        }
    }

    // ----------------------------------------------------------------------------
    // Updates position and velocity using Semi-Implicit Euler integration.
    // ----------------------------------------------------------------------------
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

        // Iterate through all entities with a player tag
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

    // ----------------------------------------------------------------------------
    // Detects and resolves collisions using direct positional correction.
    // ----------------------------------------------------------------------------
    void PhysicsSystem::ResolveCollisions(float dt) {
		EntityManager entityMgr(*ecs_);
        std::vector<Entity> allColliders = entityMgr.GetAllEntitiesWithComponents(AABBColliderType, ActiveTagType);

        // --- Check Entity Count ---
        if (allColliders.size() > kMaxCollidableEntities) { // preventive measure for too much entities
            // Log an error and fall back to narrow based implementation to prevent crashing
            static bool errorLogged = false;
            if (!errorLogged) {
                Logger::Get().Error(DebugType::PhysicsSystem)
                    << "More than " << kMaxCollidableEntities << " colliders! "
                    << "Falling back to O(n^2). Increase kMaxCollidableEntities.";
                errorLogged = true;
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

        // --- BUILD PHASE (Insert all entities into grid) ---
        // Clear all bits in the grid from the last frame
        for (std::vector<std::bitset<kMaxCollidableEntities>>::iterator::value_type& cell : grid_) {
            cell.reset();
        }

        for (int i = 0; i < colliderEntities_.size(); ++i) {
            Entity entity = colliderEntities_[i];

            // Get the AABB_Bounds for insertion
            const Vector2D& pos = entityMgr.GetComponent<Position>(entity).Pos;
            const Vector2D& halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
            float rotation = 0.0f;
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
            }
            AABB_Bounds bounds = GetRotatedAABB(pos, halfSize, rotation);

            GridInsert(i, bounds); // Insert the bitset index 'i'
        }

        // --- 3. QUERY PHASE (Check for collisions) ---
        // This set prevents checking the same pair twice (e.g., A-B and B-A)
        std::set<std::pair<Entity, Entity>> checkedPairs;
        std::bitset<kMaxCollidableEntities> potentialPartners;

        for (int i = 0; i < colliderEntities_.size(); ++i) {
            Entity e1 = colliderEntities_[i];

            // Get the AABB for e1 to query the grid
            const Vector2D& pos1 = entityMgr.GetComponent<Position>(e1).Pos;
            const Vector2D& halfSize1 = entityMgr.GetComponent<AABBCollider>(e1).HalfSize;
            float rotation1 = 0.0f;
            if (entityMgr.HasComponent(e1, RotationFlagType)) {
                rotation1 = entityMgr.GetComponent<RotationFlag>(e1).currentRotation;
            }
            AABB_Bounds bounds1 = GetRotatedAABB(pos1, halfSize1, rotation1);

            // Get all potential partners from the grid (as a bitset)
            GridQuery(bounds1, potentialPartners);

            // Now, `potentialPartners` has a '1' for the index of
            // every entity that *could* be colliding with e1.

            // Iterate only over potential partners
            // We use .test() to check if a bit is set
            for (int j = 0; j < colliderEntities_.size(); ++j) {
                if (i == j) continue; // Don't check against self

                if (potentialPartners.test(j)) {
                    Entity e2 = colliderEntities_[j];

                    // Ensure we only check each pair once
                    Entity first = std::min(e1, e2);
                    Entity second = std::max(e1, e2);

                    // .insert returns a pair. .second is true if insertion
                    // was successful (i.e., the pair was new).
                    if (checkedPairs.insert({ first, second }).second) {
                        // This is a new pair. Perform the narrow-phase check.
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
        EntityManager entityMgr(*ecs_);

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

        // --- Perform Resolution if overlapping ---
        if (contact.State == CollisionState::Overlapping) {
            // Ensure e1 is always the dynamic object for consistent resolution
            Entity dynE = e1, staticE = e2;
            if (entityMgr.GetComponent<RigidBody>(dynE).InverseMass == 0.0f) {
                std::swap(dynE, staticE); // swaps if E1 is the static object IE. Mass = 0
            }

            Vector2D& dynPos = entityMgr.GetComponent<Position>(dynE).Pos;
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

            if (minOverlapY < minOverlapX) {
                // Vertical collision has priority for platformers
                if (overlapTop < overlapBottom) {
                    dynPos.y -= overlapTop; // Push down (hit head)
                    if (dynVel.y > 0) dynVel.y = 0;
                }
                else {
                    dynPos.y += overlapBottom; // Push up (landed)
                    if (dynVel.y < 0) dynVel.y = 0;
                    if (entityMgr.HasComponent(dynE, PlayerStateType)) entityMgr.GetComponent<PlayerState>(dynE).isGrounded = true;
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

    // ------ GRID HELPER FUNCTIONS ------ //

    int PhysicsSystem::GetCellCoordX(float worldPos) const {
        int coord = static_cast<int>(std::floor((worldPos - kGridWorldMinX) * kInvCellSize));
        return std::clamp(coord, 0, kGridCellsX - 1);
    }

    
    int PhysicsSystem::GetCellCoordY(float worldPos) const {
        int coord = static_cast<int>(std::floor((worldPos - kGridWorldMinY) * kInvCellSize));
        return std::clamp(coord, 0, kGridCellsY - 1);
    }

   
    int PhysicsSystem::GetCellIndex(int x, int y) const {
        return x + y * kGridCellsX;
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

    
    void PhysicsSystem::GridQuery(const AABB_Bounds& bounds, std::bitset<kMaxCollidableEntities>& potentialColliders) const {
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

    };

} // namespace Framework