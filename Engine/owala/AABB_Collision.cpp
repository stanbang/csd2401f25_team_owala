// AABB_Collision.cpp
#include "AABB_Collision.h"
#include <algorithm> // For std::min/max

namespace Framework {

    // Implementation of the static AABB overlap test.
    bool CollisionIntersection_RectRect_Static(const AABB_Bounds& aabb1, const AABB_Bounds& aabb2) {
        bool xOverlap = (aabb1.max.x >= aabb2.min.x) && (aabb1.min.x <= aabb2.max.x);
        bool yOverlap = (aabb1.max.y >= aabb2.min.y) && (aabb1.min.y <= aabb2.max.y);
        return xOverlap && yOverlap;
    }

    // Implementation of the dynamic (swept) AABB test.
    bool CollisionIntersection_RectRect_Dynamic(
        const AABB_Bounds& aabb1, const Vector2D& vel1,
        const AABB_Bounds& aabb2, const Vector2D& vel2,
        float dt, float& firstTimeOfCollision)
    {
        // Calculate the relative velocity of object 2 with respect to object 1.
        // This simplifies the problem to a moving box and a stationary box.
        Vector2D relativeVel = vel2 - vel1;

        float tFirst = 0.0f; // The time of the first potential contact
        float tLast = dt;    // The time of the last potential contact

        // --- X-Axis Sweep ---
        if (relativeVel.x < 0.0f) {
            if (aabb1.max.x < aabb2.min.x) return false; // Separated and moving apart
            if (aabb2.max.x < aabb1.min.x) {
                tFirst = std::max((aabb2.max.x - aabb1.min.x) / relativeVel.x, tFirst);
            }
            if (aabb1.max.x > aabb2.min.x) {
                tLast = std::min((aabb1.min.x - aabb2.max.x) / relativeVel.x, tLast);
            }
        }
        else if (relativeVel.x > 0.0f) {
            if (aabb1.min.x > aabb2.max.x) return false; // Separated and moving apart
            if (aabb2.min.x > aabb1.max.x) {
                tFirst = std::max((aabb2.min.x - aabb1.max.x) / relativeVel.x, tFirst);
            }
            if (aabb1.min.x < aabb2.max.x) {
                tLast = std::min((aabb2.max.x - aabb1.min.x) / relativeVel.x, tLast);
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
                tFirst = std::max((aabb2.max.y - aabb1.min.y) / relativeVel.y, tFirst);
            }
            if (aabb1.max.y > aabb2.min.y) {
                tLast = std::min((aabb1.min.y - aabb2.max.y) / relativeVel.y, tLast);
            }
        }
        else if (relativeVel.y > 0.0f) {
            if (aabb1.min.y > aabb2.max.y) return false; // Separated and moving apart
            if (aabb2.min.y > aabb1.max.y) {
                tFirst = std::max((aabb2.min.y - aabb1.max.y) / relativeVel.y, tFirst);
            }
            if (aabb1.min.y < aabb2.max.y) {
                tLast = std::min((aabb2.max.y - aabb1.min.y) / relativeVel.y, tLast);
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