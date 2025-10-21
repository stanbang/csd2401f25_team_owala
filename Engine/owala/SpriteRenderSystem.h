/*****************************************************************//**
 * \file   SpriteRenderSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 * \brief
 *   Rendering system responsible for drawing 2D sprites and
 *   optional collider outlines. Integrates with ECS storage,
 *   Graphics, and ResourceManager to batch-draw entities.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "PhysicsSystem.h"   // included only if collider debug helpers are needed
#include <memory>

namespace Framework {

    /**
     * \class SpriteRenderSystem
     * \brief ECS-driven system that renders textured sprites for entities.
     *
     * Responsibilities:
     * - Clear the screen each frame to a configurable color
     * - Render sprites based on entity components (transform, sprite, etc.)
     * - Optionally render collider outlines for debugging
     *
     * This system ties together EntityStorage (ECS), Graphics (OpenGL wrapper),
     * and ResourceManager (for texture lookup).
     *
     * Example usage:
     * \code
     * SpriteRenderSystem srs(ecs, gfx, resources, true);
     * core.AddSystem(&srs);
     * \endcode
     */
    class SpriteRenderSystem : public ISystem {
    public:
        /**
         * \brief Construct the sprite renderer.
         * \param ecs          Reference to ECS storage for entities/components.
         * \param gfx          Reference to Graphics subsystem for drawing.
         * \param resources    Reference to ResourceManager for texture access.
         * \param drawColliders Whether to draw collider outlines each frame.
         * \param clearColor    Background clear color (RGBA).
         */
        SpriteRenderSystem(EntityStorage& ecs,
            Graphics& gfx,
            ResourceManager& resources,
            bool drawColliders = false,
            const Color& clearColor = { 0.08f, 0.10f, 0.14f, 1.0f })
            : ecs_(ecs), gfx_(gfx), resources_(resources),
            drawColliders_(drawColliders), clearColor_(clearColor) {
        }

        /**
         * \brief Initialize the render system.
         * \details No-op here, provided for ISystem compliance.
         */
        void Initialize() override {}

        /**
         * \brief Per-frame update for rendering.
         * \param dt Delta time in seconds (unused in simple rendering loop).
         * \details Clears the screen, draws sprites, and optionally draws colliders.
         */
        void Update(float dt) override;

        /**
         * \brief Shut down the render system.
         * \details No-op here, provided for ISystem compliance.
         */
        void Shutdown() override {}

        /**
         * \brief Enable or disable drawing of collider outlines.
         * \param v True to enable collider debug rendering; false to disable.
         */
        void SetDrawColliders(bool v) { drawColliders_ = v; }

        /**
         * \brief Change the background clear color.
         * \param c New RGBA clear color.
         */
        void SetClearColor(const Color& c) { clearColor_ = c; }
        /**
         * \brief Check if collider outlines are being drawn.
         * \return True if collider debug rendering is enabled.
         */
        bool IsDrawingColliders() const { return drawColliders_; }

        /**
         * \brief Toggle collider outline rendering on/off.
         */
        void ToggleColliders() { drawColliders_ = !drawColliders_; }
    private:
        /**
         * \brief Render all sprites for active entities.
         * \details Uses ECS to find entities with sprite components,
         *          fetches textures via ResourceManager, and draws via Graphics.
         */
        void RenderSprites();

        /**
         * \brief Render collider outlines for debugging.
         * \details Draws axis-aligned rectangles around entities with colliders.
         */
        void RenderColliderOutlines();

    private:
        EntityStorage& ecs_;          //!< ECS storage reference.
        Graphics& gfx_;          //!< Graphics subsystem reference.
        ResourceManager& resources_;    //!< Resource manager reference (textures).
        bool             drawColliders_ = false; //!< Flag for collider debug rendering.
        Color            clearColor_;   //!< Background clear color.
    };

} // namespace Framework
