/*******************************************************************
* \file   TransformationSystem.h
*
* \primary_author
*       Sitara Anil
*
* \brief    A system that manages real-time transformation of game
*           objects including translation, rotation, and scaling.
*           Handles global world rotation mechanics using Q and E keys.
*           Adds cinematic camera zoom: zoom out -> rotate -> zoom in.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once

#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "Matrix3x3.h"
#include "Vector2D.h"

#include <unordered_map>

namespace Framework {

    class EntityManager;

    class TransformationSystem : public ISystem {
    public:
        TransformationSystem(EntityStorage& ecs, InputSystem& input, PhysicsSystem& physics);

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        Matrix3x3 GetTransformMatrix(Entity entity) const;
        void UpdateEntityTransform(Entity entity);

        float GetGlobalRotation() const { return globalRotation; }
        bool IsGlobalRotating() const { return isGlobalRotating; }

        void ResetGlobalRotation();

        static Vector2D GetOriginalPos(EntityManager& entityMgr, Entity entity);

        // Optional to keep public; used by Update() anyway.
        void UpdateRotateCinematic();

    private:
        EntityStorage* ecs_{ nullptr };
        InputSystem* input_{ nullptr };
        PhysicsSystem* physics_{ nullptr };

        // -------------------------
        // Global rotation state
        // -------------------------
        float globalRotation{ 0.0f };
        float targetGlobalRotation{ 0.0f };
        bool  isGlobalRotating{ false };
        float rotationCooldown{ 0.0f };
        float rotationCooldownDuration{ 0.5f };

        // -------------------------
        // Cinematic zoom state machine
        // -------------------------
        enum class RotatePhase { Idle, ZoomingOut, Rotating, ZoomingIn };
        RotatePhase rotatePhase_{ RotatePhase::Idle };

        float cachedZoom_{ 1.0f };        // zoom before rotation
        float wideZoom_{ 1.0f };          // cachedZoom_ * rotateZoomFactor_
        float rotateZoomFactor_{ 1.78f }; // >1 means zoom out
        float zoomSpeed_{ 50.0f };         // exponential ease strength

        float pendingDelta_{ 0.0f };      // +90 or -90 queued rotation
        bool  zoomOutInitialized_{ false };

        // -------------------------
        // Internal helpers
        // -------------------------
        void ProcessRotationInput();
        void UpdateGlobalRotation();
        Matrix3x3 ComputeTransformMatrix(Entity entity) const;
        void UpdateAllTransforms();

        std::unordered_map<Entity, Vector2D> originalColliderSizes_;
    };

} // namespace Framework
