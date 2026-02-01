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
#include "pch.h"
#include "CameraSystem.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "OwalaTime.h"
#include "Logger.h"

namespace Framework {

    /**
     * \brief Constructs a CameraSystem and binds it to an ECS storage.
     * \param ecs Reference to the ECS storage used for entity-component management.
     */
    //CameraSystem::CameraSystem(EntityStorage& ecs) : ecs_(ecs) {
    //    // Constructor
    //}

    /**
     * \brief Updates all camera entities.
     *
     * Computes the camera’s size based on zoom and aspect ratio.
     * If a camera follows a target, interpolates its position toward
     * the target’s position using an exponential decay function.
     */
    void CameraSystem::Update() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, CameraType)) {
            Camera& camera = entityMgr.GetComponent<Camera>(e);

            // Skip cameras that are controlled by the editor
            if (camera.isEditorControlled) {
                continue;
            }

            // Keep square coordinates — no aspect ratio scaling
            camera.size.x = 2.0f * camera.zoom * camera.aspectRatio;
            camera.size.y = 2.0f * camera.zoom;

            if (camera.targetEntity != 0 && entityMgr.HasComponent(camera.targetEntity, TransformType)) {
                const Vector2D& targetPos = entityMgr.GetComponent<Transform>(camera.targetEntity).position;

                if (camera.followSpeed <= 0.0f) {
                    camera.position = targetPos;
                }
                else {
                    float t = 1.0f - std::exp(-camera.followSpeed * Framework::Time::DeltaTime());
                    camera.position.x += (targetPos.x - camera.position.x) * t;
                    camera.position.y += (targetPos.y - camera.position.y) * t;
                }
            }

            // **NEW: Clamp camera position to map bounds**
            if (camera.useMapBounds) {
                float halfWidth = camera.size.x * 0.5f;
                float halfHeight = camera.size.y * 0.5f;

                // Calculate the valid range for camera center position
                float minCameraX = camera.mapMinX + halfWidth;
                float maxCameraX = camera.mapMaxX - halfWidth;
                float minCameraY = camera.mapMinY + halfHeight;
                float maxCameraY = camera.mapMaxY - halfHeight;

                // If view is larger than map, center the camera
                if (minCameraX > maxCameraX) {
                    camera.position.x = (camera.mapMinX + camera.mapMaxX) * 0.5f;
                }
                else {
                    camera.position.x = std::clamp(camera.position.x, minCameraX, maxCameraX);
                }

                if (minCameraY > maxCameraY) {
                    camera.position.y = (camera.mapMinY + camera.mapMaxY) * 0.5f;
                }
                else {
                    camera.position.y = std::clamp(camera.position.y, minCameraY, maxCameraY);
                }
            }
        }
    }
    void CameraSystem::SetMapBounds(Entity cameraEntity, float minX, float maxX, float minY, float maxY) {
        Camera& camera = GetCamera(cameraEntity);
        camera.mapMinX = minX;
        camera.mapMaxX = maxX;
        camera.mapMinY = minY;
        camera.mapMaxY = maxY;
        camera.useMapBounds = true;
    }
    void CameraSystem::CalculateAndSetBoundsFromPlatforms(Entity cameraEntity, float padding) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        bool foundAnyPlatform = false;

        // Iterate through all entities with PlatformTag
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, PlatformTagType)) {
            if (!entityMgr.HasComponent(e, TransformType)) continue;

            foundAnyPlatform = true;
            const Transform& transform = entityMgr.GetComponent<Transform>(e);

            // Get platform bounds
            float platformMinX, platformMaxX, platformMinY, platformMaxY;

            if (entityMgr.HasComponent(e, AABBColliderType)) {
                // Use collider size if available
                const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(e);
                platformMinX = transform.position.x - collider.HalfSize.x * transform.scale.x;
                platformMaxX = transform.position.x + collider.HalfSize.x * transform.scale.x;
                platformMinY = transform.position.y - collider.HalfSize.y * transform.scale.y;
                platformMaxY = transform.position.y + collider.HalfSize.y * transform.scale.y;
            }
            else if (entityMgr.HasComponent(e, TextureComponentType)) {
                // Use texture size if no collider
                const TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(e);
                platformMinX = transform.position.x - texture.HalfSize.x * transform.scale.x;
                platformMaxX = transform.position.x + texture.HalfSize.x * transform.scale.x;
                platformMinY = transform.position.y - texture.HalfSize.y * transform.scale.y;
                platformMaxY = transform.position.y + texture.HalfSize.y * transform.scale.y;
            }
            else {
                // Just use position if no size information
                platformMinX = platformMaxX = transform.position.x;
                platformMinY = platformMaxY = transform.position.y;
            }

            // Update overall bounds
            minX = std::min(minX, platformMinX);
            maxX = std::max(maxX, platformMaxX);
            minY = std::min(minY, platformMinY);
            maxY = std::max(maxY, platformMaxY);
        }

        if (!foundAnyPlatform) {
            return;
        }

        // Apply padding
        minX -= padding;
        maxX += padding;
        minY -= padding;
        maxY += padding;

        // Set the bounds
        SetMapBounds(cameraEntity, minX, maxX, minY, maxY);
    }
    void CameraSystem::SetMapBoundsEnabled(Entity cameraEntity, bool enabled) {
        Camera& camera = GetCamera(cameraEntity);
        camera.useMapBounds = enabled;
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

        // Calculate half-widths in world space
        float halfWidth = camera.zoom * camera.aspectRatio;
        float halfHeight = camera.zoom;

        // Convert from world to NDC [-1, 1]
        screenPos.x = (worldPos.x - camera.position.x) / halfWidth;
        screenPos.y = (worldPos.y - camera.position.y) / halfHeight;

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

        // Calculate half-widths in world space
        float halfWidth = camera.zoom * camera.aspectRatio;
        float halfHeight = camera.zoom;

        // Convert from NDC [-1, 1] to world space
        worldPos.x = camera.position.x + screenPos.x * halfWidth;
        worldPos.y = camera.position.y + screenPos.y * halfHeight;

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
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

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
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

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
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
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
    Camera& CameraSystem::GetCamera(Entity cameraEntity) const {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

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
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
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

        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, CameraType)) {
            Camera& cam = entityMgr.GetComponent<Camera>(e);
            cam.aspectRatio = aspect;
        }
    }

} // namespace Framework
