// RotationSystem.h
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "Matrix3x3.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {
    class RotationSystem : public ISystem {
    public:
        RotationSystem(EntityStorage& ecs, InputSystem& input);
        void Initialize() override;
        void Update(float dt) override;
        void Shutdown() override;

    private:
        EntityStorage* ecs_;
        InputSystem* input_;

     
        float globalRotation;
        float targetGlobalRotation;
        bool isGlobalRotating;
    };
} // namespace Framework