/*********************************************************************//**
 * \file   CameraSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
 *
 * \brief
 *      Implements the CameraSystem class responsible for managing camera
 *      entities within the ECS framework. Handles camera updates,
 *      transformations between world and screen space, camera activation,
 *      and automatic following of target entities with adjustable smoothing.
 *
 * \details
 *      The CameraSystem maintains one or more cameras, supporting:
 *        - Smooth target following with exponential interpolation.
 *        - Conversion between world and normalized screen coordinates.
 *        - Automatic aspect ratio adjustment based on viewport size.
 *        - Retrieval of visible bounds for culling or rendering purposes.
 *      Each camera entity contains components like position, zoom, and
 *      aspect ratio, which are updated per frame.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "CameraSystem.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include <cmath>
#include <stdexcept>
#include "OwalaTime.h"

namespace Framework {

    /**
     * \brief Constructs a CameraSystem and binds it to an ECS storage.
     * \param ecs Reference to the ECS storage used for entity-component management.
     */
    CameraSystem::CameraSystem(EntityStorage& ecs) : ecs_(ecs) {
        // Constructor
    }

    /**
     * \brief Updates all camera entities.
     *
     * Computes the camera’s size based on zoom and aspect ratio.
     * If a camera follows a target, interpolates its position toward
     * the target’s position using an exponential decay function.
     */
    void CameraSystem::Update() {
        EntityManager entityMgr(ecs_);

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(CameraType)) {
            Camera& camera = entityMgr.GetComponent<Camera>(e);

            // Keep square coordinates — no aspect ratio scaling
            camera.size.x = 2.0f * camera.zoom * camera.aspectRatio;
            camera.size.y = 2.0f * camera.zoom;

            if (camera.targetEntity != 0 && entityMgr.HasComponent(camera.targetEntity, PositionType)) {
                const auto& targetPos = entityMgr.GetComponent<Position>(camera.targetEntity).Pos;

                if (camera.followSpeed <= 0.0f) {
                    camera.position = targetPos;
                }
                else {
                    float t = 1.0f - std::exp(-camera.followSpeed * Framework::Time::DeltaTime());
                    camera.position.x += (targetPos.x - camera.position.x) * t;
                    camera.position.y += (targetPos.y - camera.position.y) * t;
                }
            }
        }
    }

    /**
     * \brief Converts a world-space position to normalized screen-space coordinates.
     * \param worldPos Position in world coordinates.
     * \param cameraEntity The camera entity used for conversion.
     * \return Screen-space coordinates relative to camera view.
     */
    Vector2D CameraSystem::WorldToScreen(const Vector2D& worldPos, Entity cameraEntity)
    {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D screenPos;

        float halfW = camera.zoom * camera.aspectRatio;
        float halfH = camera.zoom;

        screenPos.x = (worldPos.x - camera.position.x) / halfW;
        screenPos.y = (worldPos.y - camera.position.y) / halfH;

        return screenPos;
    }

    /**
     * \brief Converts normalized screen-space coordinates to world-space.
     * \param screenPos Position in screen coordinates (typically [-1,1]).
     * \param cameraEntity The camera entity used for conversion.
     * \return World-space coordinates.
     */
    Vector2D CameraSystem::ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity)
    {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D worldPos;

        float halfW = camera.zoom * camera.aspectRatio;
        float halfH = camera.zoom;

        worldPos.x = camera.position.x + screenPos.x * halfW;
        worldPos.y = camera.position.y + screenPos.y * halfH;

        return worldPos;
    }

    /**
     * \brief Retrieves the currently active camera entity.
     * \return The entity ID of the active camera.
     */
    Entity CameraSystem::GetActiveCamera() const {
        return activeCamera_;
    }

    /**
     * \brief Sets the specified camera entity as active.
     * \param cameraEntity The entity to activate as the current camera.
     */
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

    /**
     * \brief Computes the current view bounds of a camera in world space.
     * \param camera The camera entity.
     * \param left Output reference for left bound.
     * \param right Output reference for right bound.
     * \param bottom Output reference for bottom bound.
     * \param top Output reference for top bound.
     */
    void CameraSystem::GetViewBounds(Entity camera, float& left, float& right, float& bottom, float& top) {
        EntityManager entityMgr(ecs_);

        if (entityMgr.HasComponent(camera, CameraType)) {
            Camera& cam = entityMgr.GetComponent<Camera>(camera);

            left = cam.position.x - cam.size.x * 0.5f;
            right = cam.position.x + cam.size.x * 0.5f;
            bottom = cam.position.y - cam.size.y * 0.5f;
            top = cam.position.y + cam.size.y * 0.5f;
        }
        else {
            left = -1.0f; right = 1.0f; bottom = -1.0f; top = 1.0f;
        }
    }

    /**
     * \brief Creates a new camera entity and optionally sets it as active.
     * \param position Initial world-space position of the camera.
     * \param targetEntity Entity to follow (optional).
     * \param isActive Whether to make this camera the active one.
     * \return The newly created camera entity.
     */
    Entity CameraSystem::CreateCamera(const Vector2D& position, Entity targetEntity, bool isActive) {
        EntityManager entityMgr(ecs_);
        Entity cameraEntity = entityMgr.CreateEntity();

        Camera camera;
        camera.position = position;
        camera.targetEntity = targetEntity;
        camera.isActive = isActive;
        camera.aspectRatio = 16.0f / 9.0f;

        entityMgr.AddComponent(cameraEntity, camera);

        if (isActive) {
            SetActiveCamera(cameraEntity);
        }

        return cameraEntity;
    }

    /**
     * \brief Sets the aspect ratio for a specific camera entity.
     * \param cameraEntity Camera entity.
     * \param aspectRatio Desired aspect ratio (width/height).
     */
    void CameraSystem::SetAspectRatio(Entity cameraEntity, float aspectRatio) {
        Camera& camera = GetCamera(cameraEntity);
        camera.aspectRatio = aspectRatio;
    }

    /**
     * \brief Sets the zoom level for a camera.
     * \param cameraEntity Camera entity.
     * \param zoom Zoom scale; values below 0.1 are clamped.
     */
    void CameraSystem::SetZoom(Entity cameraEntity, float zoom) {
        Camera& camera = GetCamera(cameraEntity);
        camera.zoom = std::max(0.1f, zoom);
    }

    /**
     * \brief Assigns a follow target to a camera.
     * \param cameraEntity Camera entity.
     * \param targetEntity Entity to follow.
     * \param followSpeed Exponential interpolation rate.
     */
    void CameraSystem::SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed) {
        Camera& camera = GetCamera(cameraEntity);
        camera.targetEntity = targetEntity;
        camera.followSpeed = followSpeed;
    }

    /**
     * \brief Retrieves a reference to a camera component.
     * \param cameraEntity The camera entity; uses active camera if zero.
     * \return Reference to the camera component.
     * \throws std::runtime_error if the camera does not exist.
     */
    Camera& CameraSystem::GetCamera(Entity cameraEntity) {
        EntityManager entityMgr(ecs_);

        if (cameraEntity == 0) {
            cameraEntity = activeCamera_;
        }

        if (cameraEntity == 0 || !entityMgr.HasComponent(cameraEntity, CameraType)) {
            throw std::runtime_error("CameraSystem: Camera not found");
        }

        return entityMgr.GetComponent<Camera>(cameraEntity);
    }

    /**
     * \brief Gets the position of a camera entity.
     * \param cameraEntity Camera entity; uses active camera if zero.
     * \return World-space position of the camera.
     */
    Vector2D CameraSystem::GetCameraPosition(Entity cameraEntity) const {
        EntityManager entityMgr(ecs_);
        if (cameraEntity == 0)
            cameraEntity = activeCamera_;
        if (cameraEntity == 0 || !entityMgr.HasComponent(cameraEntity, CameraType))
            return { 0.0f, 0.0f };
        return entityMgr.GetComponent<Camera>(cameraEntity).position;
    }

    /**
     * \brief Updates the aspect ratio of all cameras based on screen dimensions.
     * \param screenWidth Current viewport width.
     * \param screenHeight Current viewport height.
     */
    void CameraSystem::UpdateAspectRatio(float screenWidth, float screenHeight) {
        if (screenHeight == 0) return;
        float aspect = screenWidth / screenHeight;

        EntityManager entityMgr(ecs_);
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(CameraType)) {
            Camera& cam = entityMgr.GetComponent<Camera>(e);
            cam.aspectRatio = aspect;
        }
    }

} // namespace Framework
