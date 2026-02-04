/*******************************************************************
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
#include "LayerManager.h"

#include <filesystem> // For checking file paths

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

    void PhysicsSystem::ReloadCollisionMatrixFromFile() {
        try {
            rapidjson::Document doc;
            std::string filepath = FileIOKeys::LAYER_CONFIG_FILEPATH;

            // Try root folder first for consistency
            if (std::filesystem::exists(std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + filepath)) {
                filepath = std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + filepath;
                Logger::Get().Info(DebugType::PhysicsSystem)
                    << "Using root folder config: " << filepath << std::endl;
            }

            if (!FileIOSystem::ReadFile(filepath, doc)) {
                Logger::Get().Error(DebugType::PhysicsSystem)
                    << "Could not reload collision matrix from file: " << filepath << std::endl;
                return;
            }

            if (!doc.IsObject()) {
                Logger::Get().Error(DebugType::PhysicsSystem)
                    << "Invalid collision matrix file format" << std::endl;
                return;
            }

            // Load the collision matrix data
            if (doc.HasMember("CollisionMatrix") && doc["CollisionMatrix"].IsArray()) {
                const rapidjson::Value& matrixArray = doc["CollisionMatrix"];

                Logger::Get().Info(DebugType::PhysicsSystem)
                    << "Reloading " << matrixArray.Size() << " collision matrix entries..." << std::endl;

                // Apply saved settings
                for (rapidjson::SizeType i = 0; i < matrixArray.Size(); ++i) {
                    const auto& obj = matrixArray[i];

                    if (obj.HasMember("layer1") && obj.HasMember("layer2") && obj.HasMember("canCollide")) {
                        uint8_t layer1 = static_cast<uint8_t>(obj["layer1"].GetInt());
                        uint8_t layer2 = static_cast<uint8_t>(obj["layer2"].GetInt());
                        bool canCollide = obj["canCollide"].GetBool();

                        SetLayerCollision(
                            static_cast<CollisionLayer>(layer1),
                            static_cast<CollisionLayer>(layer2),
                            canCollide
                        );
                    }
                }

                Logger::Get().Info(DebugType::PhysicsSystem)
                    << "Successfully reloaded collision matrix from: " << filepath << std::endl;
            }
            else {
                Logger::Get().Error(DebugType::PhysicsSystem)
                    << "No CollisionMatrix found in config file" << std::endl;
            }
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::PhysicsSystem)
                << "Exception while reloading collision matrix: " << e.what() << std::endl;
        }
    }



    void PhysicsSystem::Initialize() {

        
        if (!collisionMatrixInitialized_) {
            InitializeCollisionMatrix();
            collisionMatrixInitialized_ = true;
            Logger::Get().Info(DebugType::PhysicsSystem)
                << "Collision matrix initialized with defaults" << std::endl;
        }
        else {
            Logger::Get().Info(DebugType::PhysicsSystem)
                << "Collision matrix already initialized, skipping reset" << std::endl;
        }
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

        // --- Separate dynamic and static entities ---
        std::vector<Entity> dynamicColliders;
        std::vector<Entity> staticColliders;
        dynamicColliders.reserve(allColliders.size());
        staticColliders.reserve(allColliders.size());

        for (Entity e : allColliders) {
            // Dynamic entities MUST have RigidBody with mass AND velocity
            if (entityMgr.HasComponent(e, RigidBodyType) && entityMgr.HasComponent(e, VelocityType)) {
                RigidBody& rb = entityMgr.GetComponent<RigidBody>(e);
                if (rb.InverseMass > 0.0f) {
                    dynamicColliders.push_back(e);
                    continue;
                }
            }
            // Everything else is treated as static (just needs AABB)
            staticColliders.push_back(e);
        }

        size_t totalEntities = dynamicColliders.size() + staticColliders.size();

        // --- Check Entity Count OR DEBUG TOGGLE ---
        if (forceNarrowPhase_ || totalEntities > maxCollidableEntities) {
            if (!forceNarrowPhase_ && totalEntities > maxCollidableEntities) {
                static bool errorLogged = false;
                if (!errorLogged) {
                    Logger::Get().Error(DebugType::PhysicsSystem)
                        << "More than " << maxCollidableEntities << " colliders! ";
                    errorLogged = true;
                }
            }

            // Fallback: Only check dynamic vs everything
            for (Entity dynE : dynamicColliders) {
                // Check dynamic vs dynamic
                for (Entity otherDyn : dynamicColliders) {
                    if (dynE >= otherDyn) continue;
                    CheckCollisionPair(dynE, otherDyn, dt);
                }
                // Check dynamic vs static
                for (Entity staticE : staticColliders) {
                    CheckCollisionPair(dynE, staticE, dt);
                }
            }
            return;
        }

        // --- BUILD MAPPINGS (Only entities that participate in collisions) ---
        entityToColliderIndex_.clear();
        colliderEntities_.clear();

        int index = 0;
        for (Entity e : dynamicColliders) {
            entityToColliderIndex_[e] = index++;
            colliderEntities_.push_back(e);
        }
        for (Entity e : staticColliders) {
            entityToColliderIndex_[e] = index++;
            colliderEntities_.push_back(e);
        }

        // --- Insert all entities into grid ---
        for (std::vector<std::bitset<maxCollidableEntities>>::iterator::value_type& cell : grid_) {
            cell.reset();
        }

        for (int i = 0; i < colliderEntities_.size(); ++i) {
            Entity entity = colliderEntities_[i];

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
            GridInsert(i, bounds);
        }

        // --- QUERY PHASE (Only query for dynamic entities) ---
        std::set<std::pair<Entity, Entity>> checkedPairs;
        std::bitset<maxCollidableEntities> potentialPartners;

        // Only iterate through dynamic entities for collision queries
        for (Entity dynE : dynamicColliders) {
            int i = entityToColliderIndex_[dynE];

            const Vector2D& pos1 = entityMgr.GetComponent<Transform>(dynE).position;
            const Vector2D& halfSize1 = entityMgr.GetComponent<Framework::AABBCollider>(dynE).HalfSize;
            float rotation1 = 0.0f;
            if (entityMgr.HasComponent(dynE, RotationFlagType)) {
                rotation1 = entityMgr.GetComponent<RotationFlag>(dynE).currentRotation;
            }
            AABB_Bounds bounds1 = GetRotatedAABB(pos1, halfSize1, rotation1);

            GridQuery(bounds1, potentialPartners);

            for (int j = 0; j < colliderEntities_.size(); ++j) {
                if (i == j) continue;

                if (potentialPartners.test(j)) {
                    Entity e2 = colliderEntities_[j];

                    Entity first = std::min(dynE, e2);
                    Entity second = std::max(dynE, e2);

                    if (checkedPairs.insert({ first, second }).second) {
                        CheckCollisionPair(first, second, dt);
                    }
                }
            }
        }
    }


    // ----------------------------------------------------------------------------
    // --- NARROW-PHASE implementation ---
//     Previous Implementation of resolve collisions
    // ----------------------------------------------------------------------------
    void PhysicsSystem::CheckCollisionPair(Entity e1, Entity e2, float dt) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Check entities have Transform component
        if (!entityMgr.HasComponent(e1, TransformType) || !entityMgr.HasComponent(e2, TransformType)) {
            return;
        }
        // Check entities have AABBCollider component
        if (!entityMgr.HasComponent(e1, AABBColliderType) || !entityMgr.HasComponent(e2, AABBColliderType)) {
            return;
        }

        // This prevents the trap from "resting" on the bird.
        // We let them overlap so PlayerSystem can detect the kill.
        bool isE1Trap = entityMgr.HasComponent(e1, TrapPlatformComponentType);
        bool isE2Trap = entityMgr.HasComponent(e2, TrapPlatformComponentType);
        bool isE1Enemy = entityMgr.HasComponent(e1, EnemyTagType);
        bool isE2Enemy = entityMgr.HasComponent(e2, EnemyTagType);

        if ((isE1Trap && isE2Enemy) || (isE2Trap && isE1Enemy)) {
            Entity trapEntity = isE1Trap ? e1 : e2;
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

            // If the trap is in "falling mode", ignore collision resolution.
            // This stops it from landing on the bird.
            if (trapComp.hasFallen) {
                return;
            }
        }

        // --- LAYER-BASED COLLISION CHECK ---
        if (entityMgr.GetProperties(e1).layer == CollisionLayer::Default || entityMgr.GetProperties(e2).layer == CollisionLayer::Default) {
            return;
        }

        CollisionLayer& layer1 = entityMgr.GetProperties(e1).layer;
        CollisionLayer& layer2 = entityMgr.GetProperties(e2).layer;

        if (!collisionMatrix_[static_cast<size_t>(layer1)][static_cast<size_t>(layer2)]) {
            return;
        }

        // --- CHECK IF PLAYER IS INVULNERABLE AND COLLIDING WITH DYNAMIC TRAP ---
        // Identify if one entity is player and the other is a dynamic trap
        bool isE1Player = entityMgr.HasComponent(e1, PlayerTagType);
        bool isE2Player = entityMgr.HasComponent(e2, PlayerTagType);
        
        bool isE1DynamicTrap = entityMgr.HasComponent(e1, TrapPlatformComponentType) && 
                                entityMgr.HasComponent(e1, RigidBodyType);
        bool isE2DynamicTrap = entityMgr.HasComponent(e2, TrapPlatformComponentType) && 
                                entityMgr.HasComponent(e2, RigidBodyType);

        // If player and dynamic trap are colliding, check invulnerability
        if ((isE1Player && isE2DynamicTrap) || (isE2Player && isE1DynamicTrap)) {
            Entity playerEntity = isE1Player ? e1 : e2;
            
            if (entityMgr.HasComponent(playerEntity, PlayerStateType)) {
                const PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
                if (playerState.isHurt) {
                    return; // Skip collision resolution entirely
                }
            }
        }

        // Determine which entity is dynamic (needs RigidBody + Velocity)
        bool e1IsDynamic = entityMgr.HasComponent(e1, RigidBodyType) && entityMgr.HasComponent(e1, VelocityType);
        bool e2IsDynamic = entityMgr.HasComponent(e2, RigidBodyType) && entityMgr.HasComponent(e2, VelocityType);

        if (e1IsDynamic) {
            if (entityMgr.GetComponent<RigidBody>(e1).InverseMass <= 0.0f) {
                e1IsDynamic = false;
            }
        }
        if (e2IsDynamic) {
            if (entityMgr.GetComponent<RigidBody>(e2).InverseMass <= 0.0f) {
                e2IsDynamic = false;
            }
        }

        // Detect trap entities
        bool e1IsTrap = entityMgr.HasComponent(e1, TrapPlatformComponentType);
        bool e2IsTrap = entityMgr.HasComponent(e2, TrapPlatformComponentType);

        // trap v player, make trap static to avoid player pushing
        if ((e1IsTrap && isE2Player) || (e2IsTrap && isE1Player)) {
            if (e1IsTrap) e1IsDynamic = false;
            if (e2IsTrap) e2IsDynamic = false;
        }

        // trap v trap, allows stacking (top trap lands on bottom trap)
        if (e1IsTrap && e2IsTrap && e1IsDynamic && e2IsDynamic) {
            const Vector2D& pos1 = entityMgr.GetComponent<Transform>(e1).position;
            const Vector2D& pos2 = entityMgr.GetComponent<Transform>(e2).position;
            
            if (pos1.y > pos2.y) {
                e2IsDynamic = false;
            } else {
                e1IsDynamic = false;
            }
        }

        // Skip if both are static (no rigidbody or zero mass)
        if (!e1IsDynamic && !e2IsDynamic) {
            return;
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
            // Only check dynamic collision if both entities have velocity
            if (e1IsDynamic && e2IsDynamic) {
                float t;
                Vector2D vel1 = entityMgr.GetComponent<Velocity>(e1).Vel;
                Vector2D vel2 = entityMgr.GetComponent<Velocity>(e2).Vel;
                if (CollisionIntersection_RectRect_Dynamic(bounds1, vel1, bounds2, vel2, dt, t)) {
                    contact.State = CollisionState::WillCollide;
                }
            }
        }

        // --- Perform Resolution if overlapping ---
        if (contact.State == CollisionState::Overlapping) {
            // Determine which is dynamic
            Entity dynE = e1IsDynamic ? e1 : e2;
            //Entity staticE = e1IsDynamic ? e2 : e1;

            Vector2D& dynPos = entityMgr.GetComponent<Transform>(dynE).position;
            Vector2D& dynVel = entityMgr.GetComponent<Velocity>(dynE).Vel;
            
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

            // Calculate biased overlaps (favor vertical for platformers)
            float biasedMinOverlapY = minOverlapY - EDGE_BIAS;

            if (biasedMinOverlapY < minOverlapX) {
                // VERTICAL COLLISION
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
                // HORIZONTAL COLLISION
                bool movingLeft = dynVel.x < -VELOCITY_THRESHOLD;
                bool movingRight = dynVel.x > VELOCITY_THRESHOLD;

                if (overlapLeft < overlapRight) {
                    if (movingRight || std::abs(dynVel.x) < 0.1f) {
                        dynPos.x -= (overlapLeft + EDGE_TOLERANCE);
                        if (dynVel.x > 0) dynVel.x = 0;
                    }
                }
                else {
                    if (movingLeft || std::abs(dynVel.x) < 0.1f) {
                        dynPos.x += (overlapRight + EDGE_TOLERANCE);
                        if (dynVel.x < 0) dynVel.x = 0;
                    }
                }
            }

            contacts_.push_back(contact);
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