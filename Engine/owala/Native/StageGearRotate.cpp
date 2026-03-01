#include "StageGearRotate.h"
#include "ScriptRegistry.h"

// Dependencies
#include "pch.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Components.h"
#include "TransformationSystem.h"
#include "OwalaTime.h"

REGISTER_SCRIPT(StageGearRotate, "StageGearRotate")

void StageGearRotate::OnCreate() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());
	Entity self = GetEntity();

	counterClockWise = entityMgr.GetProperties(self).name == "TurningGearL" ? true : false;
}

void StageGearRotate::OnUpdate() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();
    if (!entityMgr.HasComponent(self, TextureComponentType) || !entityMgr.HasComponent(self, TransformType)) return;

	TransformationSystem& transformSystem = *GameManager::GetInstance().GetSystems().transformation_;

	// Get current stage rotation parameters
	float currentStageRotation = transformSystem.GetGlobalRotation();
	float stageDelta = currentStageRotation - prevStageRotation;
	prevStageRotation = currentStageRotation;

	// Check if stage is currently rotating
	if (!transformSystem.IsGlobalRotating() || stageDelta == 0.f) return;

	// Determine stage rotation direction
	float stageDirection = (stageDelta > 0.f) ? 1.f : -1.f;

	// Rotate the gear if the stage is currently rotating
	float& rotationRad = entityMgr.GetComponent<Transform>(self).rotation.textureAngle;
	float rotationDeg = rotationRad * (180.f / M_PI);
	float rotationVelocity = 90.f;
	if (!counterClockWise) {
		rotationVelocity *= -1.f;
	}

	// Apply Rotation
	rotationDeg += (rotationVelocity * stageDirection) * Time::DeltaTime();
	
	// Cleanup
	rotationDeg = std::fmod(rotationDeg + 360.f, 360.f);
	rotationRad = rotationDeg * (M_PI / 180.f);
}

void StageGearRotate::OnDestroy() {
	// Logic to be called before Entity is deleted
}
