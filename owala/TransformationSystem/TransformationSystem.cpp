/*******************************************************************
* \file   TransformationSystem.cpp
*
* \primary_author
*       Sitara Anil
*
* \brief
*       Implementation of the TransformationSystem. Handles global world
*       rotation triggered by Q/E keys with smooth interpolation and cooldown.
*       Adds cinematic camera zoom: zoom out -> rotate -> zoom back in.
*********************************************************************/
#include "pch.h"
#include "TransformationSystem.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "Components.h"
#include "Transformation.h"
#include "OwalaTime.h"
#include "MessageBus.h"
#include "CameraSystem.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    TransformationSystem::TransformationSystem()
        : globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false), rotationCooldown(0.0f), rotationCooldownDuration(0.5f) {
    }

    void TransformationSystem::Initialize() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, TransformType)) {
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                UpdateEntityTransform(entity);
            }
        }
    }

    void TransformationSystem::Update() {
        ProcessRotationInput(); 
        UpdateRotateCinematic();
        UpdateGlobalRotation();
        UpdateAllTransforms();
    }

    // Cinematic: zoom out first (wait until done) -> rotate (hold zoom) -> zoom back in
    void TransformationSystem::UpdateRotateCinematic() {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem = gm.GetSystems().camera_;
        if (!cameraSystem) return;

        PhysicsSystem* physicsPtr = gm.GetSystems().physics_;
        if (!physicsPtr) return;
        PhysicsSystem& physics_ = *physicsPtr;
        EntityManager camMgr(gm.GetECS());
        Entity camEnt = cameraSystem->GetActiveCamera();
        if (camEnt == 0 || !camMgr.HasComponent(camEnt, CameraType)) return;

        Camera& cam = camMgr.GetComponent<Camera>(camEnt);

        float dt = Framework::Time::DeltaTime();
        float t = 1.0f - std::exp(-zoomSpeed_ * dt);

        switch (rotatePhase_) {
        case RotatePhase::Idle:
            return;

        case RotatePhase::ZoomingOut: {
            // cache original ONCE, not every frame
            if (!zoomOutInitialized_) {
                cachedZoom_ = cam.zoom;
                wideZoom_ = cachedZoom_ * rotateZoomFactor_;
                zoomOutInitialized_ = true;
            }

            float newZoom = cam.zoom + (wideZoom_ - cam.zoom) * t;
            cameraSystem->SetZoom(camEnt, newZoom);

            // wait until zoom-out completes, THEN start rotation
            if (std::abs(newZoom - wideZoom_) < 0.001f) {
                cameraSystem->SetZoom(camEnt, wideZoom_); // hold wide angle

                targetGlobalRotation += pendingDelta_;
                physics_.SetPaused(true);
                isGlobalRotating = true;

                MessageBus::Instance().Publish(VFXTriggerEvent{"WorldRotationEvent", true});
                MessageBus::Instance().Publish(WorldRotationEvent{});
                MessageBus::Instance().Publish(PlaySoundEvent{ "Stage_Rotate", AudioType::SFX, 1.0f, true });

                rotatePhase_ = RotatePhase::Rotating;
            }
            break;
        }

        case RotatePhase::Rotating: {
            // hold zoom wide during rotation
            cameraSystem->SetZoom(camEnt, wideZoom_);
            break;
        }

        case RotatePhase::ZoomingIn: {
            float newZoom = cam.zoom + (cachedZoom_ - cam.zoom) * t;
            cameraSystem->SetZoom(camEnt, newZoom);

            if (std::abs(newZoom - cachedZoom_) < 0.001f) {
                cameraSystem->SetZoom(camEnt, cachedZoom_);
                rotatePhase_ = RotatePhase::Idle;
                pendingDelta_ = 0.0f;
                zoomOutInitialized_ = false;
            }
            break;
        }
        }
    }

    void TransformationSystem::ProcessRotationInput() {
        GameManager& gm = GameManager::GetInstance();
        InputSystem& input_ = *gm.GetSystems().input_;
        EntityManager entityMgr(gm.GetECS());

        // Update cooldown timer
        if (rotationCooldown > 0.0f) {
            rotationCooldown -= Framework::Time::DeltaTime();
            if (rotationCooldown < 0.0f) rotationCooldown = 0.0f;
        }

        bool canRotate = false; // boolean for checking for rotation (for grounded player)

        // Find grounded active player
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerStateType)) {
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
            if (state.isGrounded) { canRotate = true; break; }
        }

        if (!canRotate) return;

        // Only allow queueing when no active rotation cinematic is running
        if (!isGlobalRotating && rotationCooldown <= 0.0f && rotatePhase_ == RotatePhase::Idle) {
            if (input_.KeyPressed(InputSystem::Key::Q)) {
                pendingDelta_ = +90.0f;
                rotatePhase_ = RotatePhase::ZoomingOut;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "Queued anticlockwise rotation (zooming out first)\n";
            }
            else if (input_.KeyPressed(InputSystem::Key::E)) {
                pendingDelta_ = -90.0f;
                rotatePhase_ = RotatePhase::ZoomingOut;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "Queued clockwise rotation (zooming out first)\n";
            }
        }
    }

    void TransformationSystem::UpdateGlobalRotation() {
        if (!isGlobalRotating) return;

        GameManager& gm = GameManager::GetInstance();
        PhysicsSystem* physicsPtr = gm.GetSystems().physics_;
        if (!physicsPtr) return;
        PhysicsSystem& physics_ = *physicsPtr;
        EntityManager entityMgr(gm.GetECS());

        float rotationSpeed = 270.0f; // Degrees per second
        float rotationThisFrame = rotationSpeed * Framework::Time::DeltaTime();

        float rotationIncrement = 0.0f;

        if (targetGlobalRotation > globalRotation) {
            rotationIncrement = std::min(rotationThisFrame, targetGlobalRotation - globalRotation);
            globalRotation += rotationIncrement;
        }
        else if (targetGlobalRotation < globalRotation) {
            rotationIncrement = -std::min(rotationThisFrame, globalRotation - targetGlobalRotation);
            globalRotation += rotationIncrement;
        }

        // Complete?
        if (std::abs(targetGlobalRotation - globalRotation) < 0.1f) {
            globalRotation = targetGlobalRotation;
            isGlobalRotating = false;
            physics_.SetPaused(false);

            // trigger zoom back in
            rotatePhase_ = RotatePhase::ZoomingIn;

            std::cout << "World rotation complete at " << globalRotation << " degrees\n";
        }

        if (std::abs(rotationIncrement) > 0.001f) {
            Matrix3x3 rotationMat = Transformation::CreateRotationMatrix(rotationIncrement);

            // Rotate all entities with RotationFlag around world origin (0,0)
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, RotationFlagType)) {
                if (!entityMgr.HasComponent(entity, TransformType)) continue;
                if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

                Vector2D originalPos = entityMgr.GetComponent<Transform>(entity).position;
                entityMgr.GetComponent<Transform>(entity).position = rotationMat * originalPos;
                // Update entity's local rotation angle (except for players/enemies)
                if (!entityMgr.HasComponent(entity, PlayerTagType) && !entityMgr.HasComponent(entity, EnemyTagType)) {
                    RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    rotationFlag.currentRotation += rotationIncrement;
                    rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
                }
            }

            // Keep player grounded after rotation
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerStateType)) {
                if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                state.isGrounded = true;
            }
        }
    }

    void TransformationSystem::ResetGlobalRotation() {
        globalRotation = 0.0f;
        targetGlobalRotation = 0.0f;
        isGlobalRotating = false;
        rotationCooldown = 0.0f;
        rotationCooldownDuration = 0.5f;

        // reset cinematic state too
        rotatePhase_ = RotatePhase::Idle;
        pendingDelta_ = 0.0f;
        zoomOutInitialized_ = false;
    }

    Vector2D TransformationSystem::GetOriginalPos(EntityManager& entityMgr, Entity entity) {
        if (!entityMgr.HasComponent(entity, RotationFlagType) ||
            !entityMgr.HasComponent(entity, TransformType))
        {
            return Vector2D(0.0f, 0.0f);
        }

        Vector2D currentPos = entityMgr.GetComponent<Transform>(entity).position;
        const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);

        float inverseAngle = -rotationFlag.currentRotation;
        Matrix3x3 inverseRotationMat = Transformation::CreateRotationMatrix(inverseAngle);

        return inverseRotationMat * currentPos;
    }

    void TransformationSystem::UpdateAllTransforms() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, TransformType)) {
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
            if (!entityMgr.HasComponent(entity, RotationFlagType)) continue;
            UpdateEntityTransform(entity);
        }
    }

    void TransformationSystem::UpdateEntityTransform(Entity entity) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(entity, TransformType) ||
            !entityMgr.HasComponent(entity, RotationFlagType)) {
            return;
        }

        Transform& transform = entityMgr.GetComponent<Transform>(entity);
        Vector2D position = transform.position;

        if (entityMgr.HasComponent(entity, AABBColliderType)) {
            Vector2D& scale = entityMgr.GetComponent<Transform>(entity).scale;
            AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);

            if (originalColliderSizes_.find(entity) != originalColliderSizes_.end()) {
                Vector2D expectedSize = Vector2D(
                    originalColliderSizes_[entity].x * scale.x,
                    originalColliderSizes_[entity].y * scale.y
                );

                if (std::abs(collider.HalfSize.x - expectedSize.x) > 0.01f ||
                    std::abs(collider.HalfSize.y - expectedSize.y) > 0.01f) {
                    originalColliderSizes_.erase(entity);
                }
            }

            if (originalColliderSizes_.find(entity) == originalColliderSizes_.end()) {
                Vector2D baseSize = collider.HalfSize;
                if (scale.x != 0) baseSize.x /= scale.x;
                if (scale.y != 0) baseSize.y /= scale.y;
                originalColliderSizes_[entity] = baseSize;
            }

            collider.HalfSize.x = originalColliderSizes_[entity].x * scale.x;
            collider.HalfSize.y = originalColliderSizes_[entity].y * scale.y;
        }
        Transform::Rotation& rotComp = transform.rotation;

        float textureRotationDegrees = rotComp.textureAngle * static_cast<float>((180.0f / M_PI));
        float colliderRotationDegrees = rotComp.colliderAngle * static_cast<float>((180.0f / M_PI));

        Vector2D textureScale = Vector2D(1.0f, 1.0f);
        if (entityMgr.HasComponent(entity, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);
            textureScale = tex.HalfSize;
        }

        Vector2D& scale = entityMgr.GetComponent<Transform>(entity).scale;
        textureScale.x *= scale.x;
        textureScale.y *= scale.y;

        rotComp.textureTransformMatrix =
            Transformation::CreateTransformationMatrix(position, textureRotationDegrees, textureScale);

        if (entityMgr.HasComponent(entity, AABBColliderType)) {
            Vector2D colliderScale = Vector2D(1.0f, 1.0f);
            colliderRotationDegrees = 0.0f;

            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                colliderRotationDegrees = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
            }
            else {
                colliderRotationDegrees = rotComp.colliderAngle * static_cast<float>((180.0f / M_PI));
            }

            Vector2D& newscale = entityMgr.GetComponent<Transform>(entity).scale;
            colliderScale.x = newscale.x;
            colliderScale.y = newscale.y;

            rotComp.colliderTransformMatrix =
                Transformation::CreateTransformationMatrix(position, colliderRotationDegrees, colliderScale);
        }

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
            float worldRotation = rotationFlag.currentRotation;

            Vector2D newscale = entityMgr.GetComponent<Transform>(entity).scale;
            rotationFlag.transformMatrix =
                Transformation::CreateTransformationMatrix(position, worldRotation, newscale);
        }
    }

    Matrix3x3 TransformationSystem::GetTransformMatrix(Entity entity) const {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            return entityMgr.GetComponent<RotationFlag>(entity).transformMatrix;
        }
        return Matrix3x3();
    }

    Matrix3x3 TransformationSystem::ComputeTransformMatrix(Entity entity) const {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(entity, TransformType)) {
            return Matrix3x3();
        }

        Vector2D position = entityMgr.GetComponent<Transform>(entity).position;
        float rotation = 0.0f;
        Vector2D scale = Vector2D(1.0f, 1.0f);

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
        }

        return Transformation::CreateTransformationMatrix(position, rotation, scale);
    }

    void TransformationSystem::Shutdown() {
    }

} // namespace Framework
