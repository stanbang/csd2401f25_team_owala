/*****************************************************************//**
 * \file   Vector2D.h
 * \brief
 *
 *
 * \author Stanley Lur
 * \date   11 September 2025
 *********************************************************************/
#pragma once
#include <cmath>

struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;

    // Constructors
    Vector2D() = default;
    Vector2D(float x_val, float y_val) : x(x_val), y(y_val) {}

    // Vector addition
    Vector2D operator+(const Vector2D& rhs) const {
        return Vector2D(x + rhs.x, y + rhs.y);
    }

    // Vector subtraction
    Vector2D operator-(const Vector2D& rhs) const {
        return Vector2D(x - rhs.x, y - rhs.y);
    }

    // ----------------------------------------------------------------------------
    // Adds the values of another vector (rhs) to this vector.
    // Returns a reference to the updated vector.
    // ----------------------------------------------------------------------------
    Vector2D& operator+=(const Vector2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    // ----------------------------------------------------------------------------
    // Subtracts the values of another vector (rhs) from this vector.
    // Returns a reference to the updated vector.
    // ----------------------------------------------------------------------------
    Vector2D& operator-=(const Vector2D& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    // ----------------------------------------------------------------------------
    // Returns the negation of this vector (i.e., -x and -y)
    // ----------------------------------------------------------------------------
    Vector2D operator-() const {
        return Vector2D(-x, -y);
    }

    // Scalar multiplication
    Vector2D operator*(float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }

    // Scalar multiplication assignment
    Vector2D& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    // Scalar division
    Vector2D operator/(float scalar) const {
        if (scalar != 0.0f) {
            return Vector2D(x / scalar, y / scalar);
        }
        return Vector2D(0.0f, 0.0f); // Return zero vector if division by zero
    }

    // ----------------------------------------------------------------------------
    // Divides this vector by a scalar value (rhs).
    // Returns a reference to the updated vector.
    // ----------------------------------------------------------------------------
    Vector2D& operator/=(float rhs) {
        if (rhs != 0.0f) {
            x /= rhs;
            y /= rhs;
        }
        return *this;
    }

    // Equality operators
    bool operator==(const Vector2D& rhs) const {
        const float epsilon = 1e-6f;
        return std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
    }

    bool operator!=(const Vector2D& rhs) const {
        return !(*this == rhs);
    }

    // ----------------------------------------------------------------------------
    // Calculate the dot product of this vector with another vector
    // Returns the scalar result of the dot product
    // ----------------------------------------------------------------------------
    float Dot(const Vector2D& rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    // ----------------------------------------------------------------------------
    // Calculate the cross product of this vector with another vector
    // In 2D, this returns the scalar z-component of the 3D cross product
    // ----------------------------------------------------------------------------
    float Cross(const Vector2D& rhs) const {
        return x * rhs.y - y * rhs.x;
    }

    // Calculate the magnitude (length) of the vector
    float Magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    // Calculate the squared magnitude 
    float MagnitudeSquared() const {
        return x * x + y * y;
    }

    // Normalize the vector (make its length 1)
    void Normalize() {
        float mag = Magnitude();
        if (mag > 0.0f) {
            x /= mag;
            y /= mag;
        }
    }

    // Return a normalized copy of this vector without modifying the original
    Vector2D Normalized() const {
        float mag = Magnitude();
        if (mag > 0.0f) {
            return Vector2D(x / mag, y / mag);
        }
        return Vector2D(0.0f, 0.0f);
    }

    // Calculate the distance between this vector and another vector
    float Distance(const Vector2D& other) const {
        return (*this - other).Magnitude();
    }

    // Calculate the squared distance (more efficient when you don't need the actual distance)
    float DistanceSquared(const Vector2D& other) const {
        return (*this - other).MagnitudeSquared();
    }

    // Rotate the vector by an angle in radians
    Vector2D Rotated(float angleRadians) const {
        float cos_a = std::cos(angleRadians);
        float sin_a = std::sin(angleRadians);
        return Vector2D(x * cos_a - y * sin_a, x * sin_a + y * cos_a);
    }

    // Rotate this vector by an angle in radians
    void Rotate(float angleRadians) {
        float cos_a = std::cos(angleRadians);
        float sin_a = std::sin(angleRadians);
        float newX = x * cos_a - y * sin_a;
        float newY = x * sin_a + y * cos_a;
        x = newX;
        y = newY;
    }

    // Get the angle of this vector in radians
    float Angle() const {
        return std::atan2(y, x);
    }

    // Linear interpolation between this vector and another
    Vector2D Lerp(const Vector2D& target, float t) const {
        return *this + (target - *this) * t;
    }

    // Clamp the magnitude of the vector to a maximum length
    Vector2D ClampMagnitude(float maxLength) const {
        float mag = Magnitude();
        if (mag > maxLength && mag > 0.0f) {
            return (*this / mag) * maxLength;
        }
        return *this;
    }

    // Project this vector onto another vector
    Vector2D Project(const Vector2D& onto) const {
        float ontoMagSq = onto.MagnitudeSquared();
        if (ontoMagSq > 0.0f) {
            return onto * (Dot(onto) / ontoMagSq);
        }
        return Vector2D(0.0f, 0.0f);
    }

    // Reflect this vector off a surface with the given normal
    Vector2D Reflect(const Vector2D& normal) const {
        return *this - normal * (2.0f * Dot(normal));
    }

    // Static utility functions
    static Vector2D Zero() { return Vector2D(0.0f, 0.0f); }
    static Vector2D One() { return Vector2D(1.0f, 1.0f); }
    static Vector2D Up() { return Vector2D(0.0f, 1.0f); }
    static Vector2D Down() { return Vector2D(0.0f, -1.0f); }
    static Vector2D Left() { return Vector2D(-1.0f, 0.0f); }
    static Vector2D Right() { return Vector2D(1.0f, 0.0f); }

}; 

// Non-member operators for scalar multiplication (scalar * vector)
inline Vector2D operator*(float scalar, const Vector2D& vec) {
    return vec * scalar;
}