/*****************************************************************//**
* \file   RotationSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief    System that handles global world rotation mechanics and
*           the entire game world to rotate in 90-degree increments using
*           Q and E keys. Pauses physics during rotation transitions.
*
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "Matrix3x3.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PhysicsSystem.h"

namespace Framework {
    class RotationSystem : public ISystem {
    public:
        RotationSystem(EntityStorage& ecs, InputSystem& input, PhysicsSystem& physics);
        void Initialize() override;
        void Update() override;
        void Shutdown() override;

    private:
        EntityStorage* ecs_;
        InputSystem* input_;
        PhysicsSystem* physics_;


        float rotationCooldown;
        float rotationCooldownDuration;
        float globalRotation;
        float targetGlobalRotation;
        bool isGlobalRotating;
    };
} // namespace Framework