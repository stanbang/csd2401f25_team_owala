/*********************************************************************//**
 * \file   CameraSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the CameraSystem class responsible for managing
 *      camera entities within the Entity-Component-System (ECS) framework.
 *      Provides functions for coordinate transformation, aspect ratio
 *      management, camera following, and active camera control.
 *
 * \details
 *      The CameraSystem class implements the ISystem interface and provides:
 *        - World-to-screen and screen-to-world coordinate conversion.
 *        - Dynamic camera creation and activation.
 *        - Camera following logic with adjustable smoothing speed.
 *        - Aspect ratio and zoom management.
 *        - Retrieval of visible view bounds for rendering and culling.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "Components.h"      ///< Includes Camera struct and related component definitions.
#include <memory>
#include "entity.h"

namespace Framework {
    class EntityStorage;
}

namespace Framework {

    /**
     * \class CameraSystem
     * \brief Handles camera behavior and transformations in the ECS framework.
     *
     * This system manages all camera components, supports following entities,
     * zoom control, aspect ratio adjustment, and coordinate conversion between
     * world and normalized screen space.
     */
    class CameraSystem : public ISystem {
    public:
        /**
         * \brief Constructs a CameraSystem instance.
         */
        CameraSystem() = default;

        /** \brief Initializes the camera system (currently unused). */
        void Initialize() override {}

        /** \brief Updates camera positions and size each frame based on zoom and follow target. */
        void Update() override;

        /** \brief Shuts down the camera system (currently unused). */
        void Shutdown() override {}

        /**
         * \brief Converts a world-space position to normalized screen-space coordinates.
         * \param worldPos Position in world coordinates.
         * \param cameraEntity Optional camera entity; uses active camera if zero.
         * \return Corresponding position in normalized screen space.
         */
        Vector2D WorldToScreen(const Vector2D& worldPos, Entity cameraEntity = 0);

        /**
         * \brief Converts normalized screen-space coordinates to world-space.
         * \param screenPos Position in screen coordinates (typically [-1, 1]).
         * \param cameraEntity Optional camera entity; uses active camera if zero.
         * \return Corresponding position in world coordinates.
         */
        Vector2D ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity = 0);

        /**
         * \brief Retrieves the currently active camera entity.
         * Validates that the stored entity still has a Camera component;
         * resets activeCamera_ to 0 if the reference has gone stale.
         * \return Entity ID of the active camera, or 0 if none is valid.
         */
        Entity GetActiveCamera();

        /**
         * \brief Sets a camera as the active camera in the system.
         * \param cameraEntity Entity ID of the camera to activate.
         */
        void SetActiveCamera(Entity cameraEntity);

        /**
         * \brief Retrieves the camera's current visible bounds in world space.
         * \param cameraEntity Camera entity to query.
         * \param left Output left boundary.
         * \param right Output right boundary.
         * \param bottom Output bottom boundary.
         * \param top Output top boundary.
         */
        void GetViewBounds(Entity cameraEntity, float& left, float& right, float& bottom, float& top);

        /**
         * \brief Creates a new camera entity and optionally sets it as active.
         * \param position Initial world-space position of the camera.
         * \param targetEntity Entity to follow (optional).
         * \param isActive Whether this camera should be active on creation.
         * \return Entity ID of the newly created camera.
         */
        Entity CreateCamera(const Vector2D& position = { 0.0f, 0.0f },
            Entity targetEntity = 0,
            bool isActive = false);

        /**
         * \brief Sets the aspect ratio of a given camera.
         * \param cameraEntity Camera entity.
         * \param aspectRatio Desired aspect ratio (width/height).
         */
        void SetAspectRatio(Entity cameraEntity, float aspectRatio);

        /**
         * \brief Sets the zoom level for a given camera.
         * \param cameraEntity Camera entity.
         * \param zoom Zoom level; values below 0.1 are clamped.
         */
        void SetZoom(Entity cameraEntity, float zoom);

        /**
         * \brief Assigns a target entity for the camera to follow.
         * \param cameraEntity Camera entity.
         * \param targetEntity Entity to follow.
         * \param followSpeed Speed of interpolation toward the target.
         */
        void SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed = 5.0f);

        /**
         * \brief Retrieves the current world-space position of a camera.
         * \param cameraEntity Camera entity; uses active camera if zero.
         * \return World-space position vector.
         */
        Vector2D GetCameraPosition(Entity cameraEntity) const;

        /**
         * \brief Retrieves a reference to the camera component of an entity.
         * \param cameraEntity Camera entity; uses active camera if zero.
         * \return Reference to the Camera component.
         * \throws std::runtime_error if the camera does not exist.
         */
        Camera& GetCamera(Entity cameraEntity) const;

        /**
         * \brief Updates aspect ratio for all cameras based on viewport dimensions.
         * \param screenWidth Width of the screen in pixels.
         * \param screenHeight Height of the screen in pixels.
         */
        void UpdateAspectRatio(float screenWidth, float screenHeight);
        /**
         * \brief Sets the map boundaries for camera clamping.
         * \param cameraEntity Camera entity.
         * \param minX Minimum X boundary of the map.
         * \param maxX Maximum X boundary of the map.
         * \param minY Minimum Y boundary of the map.
         * \param maxY Maximum Y boundary of the map.
         */
        void SetMapBounds(Entity cameraEntity, float minX, float maxX, float minY, float maxY);

        /**
         * \brief Enables or disables map bounds clamping.
         * \param cameraEntity Camera entity.
         * \param enabled Whether to enable bounds clamping.
         */
        void SetMapBoundsEnabled(Entity cameraEntity, bool enabled);
        /**
         * \brief Calculates and sets map bounds based on all platform entities in the scene.
         * \param cameraEntity Camera entity to set bounds for.
         * \param padding Optional padding around the calculated bounds (default: 0.0f).
         */
        void CalculateAndSetBoundsFromPlatforms(Entity cameraEntity, float padding = 0.0f);
    private:
        Entity activeCamera_ = 0;   ///< Currently active camera entity.
    };

} // namespace Framework
