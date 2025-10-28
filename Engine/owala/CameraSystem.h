#pragma once
#include "ISystem.h"
#include "Components.h"  // ? THIS IS CRITICAL - includes Camera struct
#include <memory>
#include "entity.h"
#include "Components.h"

namespace Framework {
    class EntityStorage;
}

namespace Framework {

    class CameraSystem : public ISystem {
    public:
        CameraSystem(EntityStorage& ecs);

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        Vector2D WorldToScreen(const Vector2D& worldPos, Entity cameraEntity = 0);
        Vector2D ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity = 0);
        Entity GetActiveCamera() const;
        void SetActiveCamera(Entity cameraEntity);
        void GetViewBounds(Entity cameraEntity, float& left, float& right, float& bottom, float& top);
        Entity CreateCamera(const Vector2D& position = { 0.0f, 0.0f },
            Entity targetEntity = 0,
            bool isActive = false);

        void SetAspectRatio(Entity cameraEntity, float aspectRatio); 
        void SetZoom(Entity cameraEntity, float zoom);
        void SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed = 5.0f);
        Vector2D GetCameraPosition(Entity cameraEntity) const;
        Camera& GetCamera(Entity cameraEntity);


    private:


        EntityStorage& ecs_;
        Entity activeCamera_ = 0;
    };

} // namespace Framework