#pragma once
#include "Vector2D.h"
#include "Entity.h"
#include <algorithm> // For std::min and std::max
#include <cmath>     // For std::abs

namespace Framework {

    // An Axis-Aligned Bounding Box, defined by its center and half-sizes.
    struct AABB {
        Vector2D Position;
        Vector2D HalfSize;
    };

    // Holds all information about a collision between two entities.
    struct Contact {
        Entity A;
        Entity B;
        Vector2D Normal;      // Direction to push B to resolve collision
        float Penetration;    // How deep the objects are overlapping
    };

    // NEW: Collision states based on your request
    enum class CollisionState {
        NoCollision,
        Overlapping, // Penetrating
        Separating   // Moving away from each other
    };

    // NEW: A robust AABB vs AABB check adapted from your provided code.
    // This is a "swept" test that considers velocity over the frame.
    inline bool SweptAABB(const AABB& a, const Vector2D& vel_a, const AABB& b, const Vector2D& vel_b, float dt, Contact& contact) {

        // --- Step 1: Static Check (Are they already overlapping?) ---
        Vector2D n = b.Position - a.Position;
        Vector2D ab_half_sizes = a.HalfSize + b.HalfSize;

        float x_overlap = ab_half_sizes.x - std::abs(n.x);
        if (x_overlap <= 0) {
            // No overlap on X axis
        }

        float y_overlap = ab_half_sizes.y - std::abs(n.y);
        if (y_overlap <= 0) {
            // No overlap on Y axis
        }

        if (x_overlap > 0 && y_overlap > 0) {
            // They are currently overlapping. This is the simplest case.
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

        // --- Step 2: Dynamic Check (Will they collide this frame?) ---
        Vector2D rel_vel = vel_b - vel_a;

        float t_first = 0.0f;
        float t_last = dt;

        // X-axis sweep
        if (rel_vel.x < 0.0f) {
            if (b.Position.x + b.HalfSize.x < a.Position.x - a.HalfSize.x) return false; // Separated and moving apart
            if (a.Position.x - a.HalfSize.x < b.Position.x + b.HalfSize.x) {
                t_first = std::max((a.Position.x + a.HalfSize.x - (b.Position.x - b.HalfSize.x)) / rel_vel.x, t_first);
            }
            if (b.Position.x + b.HalfSize.x > a.Position.x - a.HalfSize.x) {
                t_last = std::min((a.Position.x - a.HalfSize.x - (b.Position.x + b.HalfSize.x)) / rel_vel.x, t_last);
            }
        }
        else if (rel_vel.x > 0.0f) {
            if (b.Position.x - b.HalfSize.x > a.Position.x + a.HalfSize.x) return false; // Separated and moving apart
            if (b.Position.x - b.HalfSize.x < a.Position.x + a.HalfSize.x) {
                t_first = std::max((a.Position.x - a.HalfSize.x - (b.Position.x + b.HalfSize.x)) / rel_vel.x, t_first);
            }
            if (a.Position.x + a.HalfSize.x > b.Position.x - b.HalfSize.x) {
                t_last = std::min((a.Position.x + a.HalfSize.x - (b.Position.x - b.HalfSize.x)) / rel_vel.x, t_last);
            }
        }

        if (t_first > t_last) return false;

        // Y-axis sweep
        if (rel_vel.y < 0.0f) {
            if (b.Position.y + b.HalfSize.y < a.Position.y - a.HalfSize.y) return false; // Separated and moving apart
            if (a.Position.y - a.HalfSize.y < b.Position.y + b.HalfSize.y) {
                t_first = std::max((a.Position.y + a.HalfSize.y - (b.Position.y - b.HalfSize.y)) / rel_vel.y, t_first);
            }
            if (b.Position.y + b.HalfSize.y > a.Position.y - a.HalfSize.y) {
                t_last = std::min((a.Position.y - a.HalfSize.y - (b.Position.y + b.HalfSize.y)) / rel_vel.y, t_last);
            }
        }
        else if (rel_vel.y > 0.0f) {
            if (b.Position.y - b.HalfSize.y > a.Position.y + a.HalfSize.y) return false; // Separated and moving apart
            if (b.Position.y - b.HalfSize.y < a.Position.y + a.HalfSize.y) {
                t_first = std::max((a.Position.y - a.HalfSize.y - (b.Position.y + b.HalfSize.y)) / rel_vel.y, t_first);
            }
            if (a.Position.y + a.HalfSize.y > b.Position.y - b.HalfSize.y) {
                t_last = std::min((a.Position.y + a.HalfSize.y - (b.Position.y - b.HalfSize.y)) / rel_vel.y, t_last);
            }
        }

        if (t_first > t_last) return false;

        // If we reach here, a collision is predicted to happen.
        // For our discrete solver, we will handle it like a static collision.
        // A continuous solver would use `t_first` to step time forward precisely.
        // For now, we just need to know it will happen and get the normal.

        Vector2D contactPoint = a.Position + vel_a * t_first;
        Vector2D contactNormal;

        if (x_overlap > y_overlap) {
            contact.Normal = { (rel_vel.x < 0) ? 1.0f : -1.0f, 0.0f };
        }
        else {
            contact.Normal = { 0.0f, (rel_vel.y < 0) ? 1.0f : -1.0f };
        }
        contact.Penetration = 0; // The sweep test finds collision *before* penetration.

        return true;
    }
} // namespace Framework
