/*****************************************************************//**
 * \file   AABB_Collision.cpp
 *
 * \primary_author
 *      Lur Pei Hao Stanley
 * \secondary_author
 *
 *
 * \brief Implementation of static and dynamic collision functions
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "AABB_Collision.h"

namespace Framework {

    // ----------------------------------------------------------------------------
    // Performs a simple overlap test on two AABBs.
    // It checks for overlap on both the X and Y axes independently.
    // A collision occurs only if there is an overlap on both axes.
    // ----------------------------------------------------------------------------
    bool CollisionIntersection_RectRect_Static(const AABB_Bounds& aabb1, const AABB_Bounds& aabb2) {
        bool xOverlap = (aabb1.max.x >= aabb2.min.x) && (aabb1.min.x <= aabb2.max.x);
        bool yOverlap = (aabb1.max.y >= aabb2.min.y) && (aabb1.min.y <= aabb2.max.y);
        return xOverlap && yOverlap;
    }

    // ----------------------------------------------------------------------------
   // Implementation of the dynamic AABB test.
   // This  projects the bounding boxes along their relative velocity
   // to find the precise time of first and last contact.
   // ----------------------------------------------------------------------------
    bool CollisionIntersection_RectRect_Dynamic(
        const AABB_Bounds& aabb1, const Vector2D& vel1,
        const AABB_Bounds& aabb2, const Vector2D& vel2,
        float dt, float& firstTimeOfCollision)
    {
        // Calculate the relative velocity of object 2 with respect to object 1.
        Vector2D relativeVel = vel2 - vel1;

        float tFirst = 0.0f; // The time of the first potential contact
        float tLast = dt;    // The time of the last potential contact

        // --- X-Axis  ---
        if (relativeVel.x < 0.0f) {
            if (aabb1.max.x < aabb2.min.x) return false; // Separated and moving apart
            if (aabb2.max.x < aabb1.min.x) {
                tFirst = (std::max)((aabb2.max.x - aabb1.min.x) / relativeVel.x, tFirst);
            }
            if (aabb1.max.x > aabb2.min.x) {
                tLast = (std::min)((aabb1.min.x - aabb2.max.x) / relativeVel.x, tLast);
            }
        }
        else if (relativeVel.x > 0.0f) {
            if (aabb1.min.x > aabb2.max.x) return false; // Separated and moving apart
            if (aabb2.min.x > aabb1.max.x) {
                tFirst = (std::max)((aabb2.min.x - aabb1.max.x) / relativeVel.x, tFirst);
            }
            if (aabb1.min.x < aabb2.max.x) {
                tLast = (std::min)((aabb2.max.x - aabb1.min.x) / relativeVel.x, tLast);
            }
        }
        else { // No relative motion on the X-axis
            if (aabb1.max.x < aabb2.min.x || aabb1.min.x > aabb2.max.x) return false; // Static gap
        }

        if (tFirst > tLast) return false; // Collision interval is invalid

        // --- Y-Axis Sweep ---
        if (relativeVel.y < 0.0f) {
            if (aabb1.max.y < aabb2.min.y) return false; // Separated and moving apart
            if (aabb2.max.y < aabb1.min.y) {
                tFirst = (std::max)((aabb2.max.y - aabb1.min.y) / relativeVel.y, tFirst);
            }
            if (aabb1.max.y > aabb2.min.y) {
                tLast = (std::min)((aabb1.min.y - aabb2.max.y) / relativeVel.y, tLast);
            }
        }
        else if (relativeVel.y > 0.0f) {
            if (aabb1.min.y > aabb2.max.y) return false; // Separated and moving apart
            if (aabb2.min.y > aabb1.max.y) {
                tFirst = (std::max)((aabb2.min.y - aabb1.max.y) / relativeVel.y, tFirst);
            }
            if (aabb1.min.y < aabb2.max.y) {
                tLast = (std::min)((aabb2.max.y - aabb1.min.y) / relativeVel.y, tLast);
            }
        }
        else { // No relative motion on the Y-axis
            if (aabb1.max.y < aabb2.min.y || aabb1.min.y > aabb2.max.y) return false; // Static gap
        }

        if (tFirst > tLast) return false; // Final collision interval is invalid

        // A valid collision is predicted to occur.
        firstTimeOfCollision = tFirst;
        return true;
    }
}