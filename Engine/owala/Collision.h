#pragma once
#include "Vector2D.h"
#include "Entity.h"

namespace Framework {

    // An Axis-Aligned Bounding Box, defined by its center and half-sizes.
    struct AABB {
        Vector2D Position;
        Vector2D HalfSize;
    };

    // Holds all information about a collision between two entities.
    // This is often called a "contact manifold".
    struct Contact {
        Entity A;
        Entity B;
        Vector2D Normal;      // Direction to push B to resolve collision
        float Penetration;    // How deep the objects are overlapping
    };

    // Collision detection function for two AABBs.
    // Returns true if they intersect and fills the 'contact' struct.
    inline bool AABBvsAABB(const AABB& a, const AABB& b, Contact& contact) {
        Vector2D n = b.Position - a.Position; // Vector from A to B
        Vector2D ab_half_sizes = a.HalfSize + b.HalfSize;

        float x_overlap = ab_half_sizes.x - std::abs(n.x);
        if (x_overlap <= 0) return false;

        float y_overlap = ab_half_sizes.y - std::abs(n.y);
        if (y_overlap <= 0) return false;

        // The smallest overlap determines the penetration axis and depth.
        if (x_overlap < y_overlap) {
            contact.Penetration = x_overlap;
            contact.Normal = (n.x < 0) ? Vector2D(1, 0) : Vector2D(-1, 0);
        }
        else {
            contact.Penetration = y_overlap;
            contact.Normal = (n.y < 0) ? Vector2D(0, 1) : Vector2D(0, -1);
        }

        return true;
    }

} // namespace Framework
