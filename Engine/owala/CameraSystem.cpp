#include "CameraSystem.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include <cmath>
#include <stdexcept>

namespace Framework {

    CameraSystem::CameraSystem(EntityStorage& ecs) : ecs_(ecs) {}

    void CameraSystem::Update(float dt) {
        EntityManager entityMgr(ecs_);

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(CameraType)) {
            Camera& camera = entityMgr.GetComponent<Camera>(e);

            // Camera size depends only on zoom (aspect-free)
            camera.size = { 2.0f * camera.zoom, 2.0f * camera.zoom };

            // Follow target logic (unchanged)
            if (camera.targetEntity != 0 && entityMgr.HasComponent(camera.targetEntity, PositionType)) {
                const auto& targetPos = entityMgr.GetComponent<Position>(camera.targetEntity).Pos;

                if (camera.followSpeed <= 0.0f) {
                    camera.position = targetPos;
                }
                else {
                    float t = 1.0f - std::exp(-camera.followSpeed * dt);
                    camera.position.x += (targetPos.x - camera.position.x) * t;
                    camera.position.y += (targetPos.y - camera.position.y) * t;
                }
            }
        }
    }

    Vector2D CameraSystem::WorldToScreen(const Vector2D& worldPos, Entity cameraEntity) {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D screenPos;

        // Convert world ? normalized screen coordinates
        // Only transform position, don't affect size
        /*screenPos.x = (worldPos.x - camera.position.x) / (camera.size.x * 0.5f * camera.zoom);
        screenPos.y = (worldPos.y - camera.position.y) / (camera.size.y * 0.5f * camera.zoom);*/

        screenPos.x = (worldPos.x - camera.position.x) / (camera.size.x * 0.5f);
        screenPos.y = (worldPos.y - camera.position.y) / (camera.size.y * 0.5f);

        return screenPos;
    }

    Vector2D CameraSystem::ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity) {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D worldPos;

        worldPos.x = camera.position.x + (screenPos.x * camera.size.x * 0.5f);
        worldPos.y = camera.position.y + (screenPos.y * camera.size.y * 0.5f);

        return worldPos;
    }

    Entity CameraSystem::GetActiveCamera() const {
        return activeCamera_;
    }

    void CameraSystem::SetActiveCamera(Entity cameraEntity) {
        EntityManager entityMgr(ecs_);

        if (activeCamera_ != 0 && entityMgr.HasComponent(activeCamera_, CameraType)) {
            entityMgr.GetComponent<Camera>(activeCamera_).isActive = false;
        }

        if (cameraEntity != 0 && entityMgr.HasComponent(cameraEntity, CameraType)) {
            entityMgr.GetComponent<Camera>(cameraEntity).isActive = true;
            activeCamera_ = cameraEntity;
        }
        else {
            activeCamera_ = 0;
        }
    }

    void CameraSystem::GetViewBounds(Entity camera, float& left, float& right, float& bottom, float& top) {
        EntityManager entityMgr(ecs_);

        if (entityMgr.HasComponent(camera, CameraType)) {
            Camera& cam = entityMgr.GetComponent<Camera>(camera);

            // Simplified aspect-free bounds
            float halfWidth = cam.size.x * 0.5f;
            float halfHeight = cam.size.y * 0.5f;

            left = cam.position.x - halfWidth;
            right = cam.position.x + halfWidth;
            bottom = cam.position.y - halfHeight;
            top = cam.position.y + halfHeight;
        }
        else {
            left = -1.0f; right = 1.0f; bottom = -1.0f; top = 1.0f;
        }
    }

    Entity CameraSystem::CreateCamera(const Vector2D& position, Entity targetEntity, bool isActive) {
        EntityManager entityMgr(ecs_);
        Entity cameraEntity = entityMgr.CreateEntity();

        Camera camera;
        camera.position = position;
        camera.targetEntity = targetEntity;
        camera.isActive = isActive;

        // REMOVE: aspectRatio no longer used
        // camera.aspectRatio = 16.0f / 9.0f;

        entityMgr.AddComponent(cameraEntity, camera);

        if (isActive) {
            SetActiveCamera(cameraEntity);
        }

        return cameraEntity;
    }

    void CameraSystem::SetZoom(Entity cameraEntity, float zoom) {
        Camera& camera = GetCamera(cameraEntity);
        camera.zoom = std::max(0.1f, zoom);
    }

    void CameraSystem::SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed) {
        Camera& camera = GetCamera(cameraEntity);
        camera.targetEntity = targetEntity;
        camera.followSpeed = followSpeed;
    }
    const Camera& CameraSystem::GetCameraConst(Entity cameraEntity) const {
        EntityManager entityMgr(ecs_);

        Entity camEntity = cameraEntity ? cameraEntity : activeCamera_;
        if (camEntity == 0 || !entityMgr.HasComponent(camEntity, CameraType)) {
            throw std::runtime_error("CameraSystem::GetCameraConst - Camera entity not found or invalid");
        }

        return entityMgr.GetComponent<Camera>(camEntity);
    }

    Camera& CameraSystem::GetCamera(Entity cameraEntity) {
        EntityManager entityMgr(ecs_);

        if (cameraEntity == 0)
            cameraEntity = activeCamera_;

        if (cameraEntity == 0 || !entityMgr.HasComponent(cameraEntity, CameraType))
            throw std::runtime_error("CameraSystem: Camera not found");

        return entityMgr.GetComponent<Camera>(cameraEntity);
    }

} // namespace Framework
