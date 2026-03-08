/*****************************************************************//**
 * \file   AABB_Collision.h
 *
 * \primary_author
 *      Lur Pei Hao Stanley
 * \secondary_author
 *
 *
 * \brief Declaration of static and dynamic collision functions
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "Vector2D.h"

namespace Framework {

    // ----------------------------------------------------------------------------
    //
    // AABB collider structure
    //
    // ----------------------------------------------------------------------------
    struct AABB_Bounds {
        Vector2D min;
        Vector2D max;
    };


    /**
     * @brief Performs a static AABB collision check to see if two boxes are currently overlapping.
     * @param aabb1 The first bounding box.
     * @param aabb2 The second bounding box.
     * @return True if the boxes overlap, false otherwise.
     */
    bool CollisionIntersection_RectRect_Static(const AABB_Bounds& aabb1, const AABB_Bounds& aabb2);

    /**
    * @brief Performs a dynamic AABB collision check to predict if two moving boxes will collide.
    * @param aabb1 The first bounding box.
    * @param vel1 The velocity of the first object.
    * @param aabb2 The second bounding box.
    * @param vel2 The velocity of the second object.
    * @param dt The time duration of the frame (timestep).
    * @param firstTimeOfCollision [out] If a collision is predicted, this will be the time of impact (from 0 to dt).
    * @return True if a collision is predicted to happen within the timestep, false otherwise.
    */
    bool CollisionIntersection_RectRect_Dynamic(
        const AABB_Bounds& aabb1, const Vector2D& vel1,
        const AABB_Bounds& aabb2, const Vector2D& vel2,
        float dt, float& firstTimeOfCollision
    );
}