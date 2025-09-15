#pragma once
#include "Vector2D.h"

namespace Framework {

    // The AABB struct your collision functions expect, using min/max points.
    struct AABB_Bounds {
        Vector2D min;
        Vector2D max;
    };

    bool CollisionIntersection_RectRect_Static(const AABB_Bounds& aabb1, const AABB_Bounds& aabb2);

    bool CollisionIntersection_RectRect_Dynamic(
        const AABB_Bounds& aabb1, const Vector2D& vel1,
        const AABB_Bounds& aabb2, const Vector2D& vel2,
        float dt, float& firstTimeOfCollision
    );
}