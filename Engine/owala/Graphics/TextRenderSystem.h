/*****************************************************************//**
 * \file   TextRenderSystem.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 * \brief
 *   Rendering system responsible for drawing text
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "ResourceManager.h"
#include "EntityManager.h"

namespace Framework {
    class TextRenderSystem : public ISystem {
    public:
        /**
         * \brief Construct the text renderer.
         * \param ecs          Reference to ECS storage for entities/components.
         * \param rcm          Reference to ResourceManager for assets access.
         * \param gfx          Reference to Graphics subsystem for drawing.
         */
        TextRenderSystem(EntityStorage& ecs, ResourceManager& rcm, Graphics& gfx, WindowsSystem& win)
            : ecs_(ecs), rcm_(rcm), gfx_(gfx), win_(win) {}

        // Camera setter
        void SetCameraSystem(CameraSystem* cameraSystem) {
            cameraSystem_ = cameraSystem;
        }

        /**
         * \brief Initialize the render system.
         * \details No-op here, provided for ISystem compliance.
         */
        void Initialize() override {}

        /**
         * \brief Per-frame update for rendering.
         */
        void Update() override;

        /**
         * \brief Shut down the render system.
         * \details No-op here, provided for ISystem compliance.
         */
        void Shutdown() override {}
    private:

        // Main text rendering function. calls graphics system to draw each line of text
        void RenderText(const Entity en, EntityManager& entityMgr, const TextComponent& textComp, const Entity camera);

        // Calculate and update text halfsize if needed
        void UpdateTextBounds(Entity entity, EntityManager& entityMgr, TextComponent& textComp, const Entity camera);

        // Calculate and update wrapped lines if needed
        void UpdateWrappedText(TextComponent& textComp);


    private:
        EntityStorage& ecs_;          //!< ECS storage reference.
        Graphics& gfx_;          //!< Graphics subsystem reference.
        ResourceManager& rcm_;    //!< Resource manager reference.
        WindowsSystem& win_;
        CameraSystem* cameraSystem_ = nullptr;
    };

} // namespace Framework
