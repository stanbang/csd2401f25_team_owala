/*****************************************************************//**
 * \file   SpriteRenderSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "PhysicsSystem.h"   // only if you want the collider debug helpers
#include <memory>

namespace Framework {

    class SpriteRenderSystem : public ISystem {
    public:
        SpriteRenderSystem(EntityStorage& ecs,
            Graphics& gfx,
            ResourceManager& resources,
            bool drawColliders = false,
            const Color& clearColor = { 0.08f, 0.10f, 0.14f, 1.0f })
            : ecs_(ecs), gfx_(gfx), resources_(resources),
            drawColliders_(drawColliders), clearColor_(clearColor) {
        }

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        void SetDrawColliders(bool v) { drawColliders_ = v; }
        void SetClearColor(const Color& c) { clearColor_ = c; }

    private:
        void RenderSprites();
        void RenderColliderOutlines();

    private:
        EntityStorage& ecs_;
        Graphics& gfx_;
        ResourceManager& resources_;
        bool drawColliders_ = false;
        Color clearColor_;
    };

} // namespace Framework
