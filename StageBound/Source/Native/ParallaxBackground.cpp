/******************************************************************
* \file   ParallaxBackground.cpp
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief	Implements the ParallaxBackground script to create a parallax scrolling effect for 
*           background layers in the scene. Each layer moves at a different speed relative to 
*           the camera, creating an illusion of depth and immersion. The parallax factor is 
*           derived from the entity's render order, allowing artists to control the depth 
*           effect by simply adjusting the render order of background entities.
*
* \copyright Copyright(C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
* ********************************************************************/

#include "ParallaxBackground.h"
#include "ScriptRegistry.h"

#include "GameManager.h"
#include "EntityManager.h"
#include "CameraSystem.h"

REGISTER_SCRIPT(ParallaxBackground, "ParallaxBackground")

// ---- Render-order range for all background layers in the scene ----
// Backmost layer  (e.g. "Background", order -100) - minimum parallax movement
// Frontmost layer (e.g. "BG_1",       order  -93) - maximum parallax movement
static constexpr int   BG_BACKMOST_ORDER  = -100;
static constexpr int   BG_FRONTMOST_ORDER =  -93;
static constexpr float BG_MIN_PARALLAX    =  0.0f;  // Fraction of camera offset for the deepest layer
static constexpr float BG_MAX_PARALLAX    =  0.1f;  // Fraction of camera offset for the closest bg layer

void ParallaxBackground::OnCreate() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();

    if (!entityMgr.HasComponent(self, TransformType)) return;

    // Record the entity's starting world position as the parallax origin
    basePosition_ = entityMgr.GetComponent<Transform>(self).position;

    // Derive parallax strength from the entity's render depth:
    // deeper layers receive a smaller factor and therefore lag further behind
    // the camera, reinforcing the sense of distance.
    const EntityProperties& props = entityMgr.GetProperties(self);
    const int renderOrder = props.customRenderOrder;

	// Calculate parallax factor based on render order within the defined background range
    const float depthRange = static_cast<float>(BG_FRONTMOST_ORDER - BG_BACKMOST_ORDER);

	// Only calculate if the render order is within the expected range to avoid unintended parallax effects on non-background entities
    if (depthRange > 0.f) {
		float t = static_cast<float>(renderOrder - BG_BACKMOST_ORDER) / depthRange; // t will be 0 for the backmost layer and 1 for the frontmost layer, with linear interpolation in between
		t = std::clamp(t, 0.f, 1.f); // Clamp to ensure it stays within bounds even if render order is outside the expected range
		parallaxFactor_ = BG_MIN_PARALLAX + t * (BG_MAX_PARALLAX - BG_MIN_PARALLAX); // Linear interpolation between min and max parallax based on render order
    }
}

void ParallaxBackground::OnUpdate() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();

    if (!entityMgr.HasComponent(self, TransformType)) return;

    CameraSystem* cameraSys = GameManager::GetInstance().GetSystems().camera_;
    if (!cameraSys) return;

    Entity activeCamera = cameraSys->GetActiveCamera();
    if (activeCamera == 0) return;

    // Offset the layer by a fraction of the camera's displacement from the origin.
    // Layers with a lower parallaxFactor_ lag further behind the camera,
    // making them appear more distant.
    const Vector2D camPos = cameraSys->GetCameraPosition(activeCamera);
    Transform& transform  = entityMgr.GetComponent<Transform>(self);
    transform.position.x  = basePosition_.x + camPos.x * parallaxFactor_;
    transform.position.y  = basePosition_.y + camPos.y * parallaxFactor_;
}

void ParallaxBackground::OnDestroy() {
}
