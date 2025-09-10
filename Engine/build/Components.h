#pragma once
#include "Vector2D.h"

// Contains position, rotation, and scale for an entity.
// This component will be read by the Graphics system and written to by the Physics system.
struct TransformComponent {
    Vector2D position;
    float rotation = 0.0f; // in radians
    Vector2D scale = { 1.0f, 1.0f };
};

// Contains all properties needed for physics simulation.
// This component is exclusively managed by the PhysicsSystem.
struct RigidBodyComponent {
    Vector2D velocity;
    Vector2D acceleration;
    float mass = 1.0f;
    float inverseMass = 1.0f;

    // Store the sum of all forces acting on the body for a single frame.
    Vector2D force;

    void SetMass(float newMass) {
        mass = newMass;
        if (mass > 0.0f) {
            inverseMass = 1.0f / mass;
        }
        else {
            inverseMass = 0.0f; // Infinite mass for static objects
        }
    }
};
