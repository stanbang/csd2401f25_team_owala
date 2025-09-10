#pragma once
#pragma once
#include <cmath>

struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;

    // Constructors
    Vector2D() = default;
    Vector2D(float x_val, float y_val) : x(x_val), y(y_val) {}

    // Vector addition
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }

    // Vector addition assignment
    Vector2D& operator+=(const Vector2D& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // Vector subtraction
    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }

    // Scalar multiplication
    Vector2D operator*(float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }

    // Calculate the magnitude (length) of the vector
    float Magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    // Normalize the vector (make its length 1)
    void Normalize() {
        float mag = Magnitude();
        if (mag > 0) {
            x /= mag;
            y /= mag;
        }
    }
};