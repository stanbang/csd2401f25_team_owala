/*****************************************************************//**
* \file   Matrix3x3.h
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief Math library for 3x3 matrix operations (2D transforms with homogeneous coordinates).
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Vector2D.h"
#include <cmath>

// Represents a 3x3 matrix in row-major order
// [ m0 m1 m2 ]
// [ m3 m4 m5 ]
// [ m6 m7 m8 ]
struct Matrix3x3 {
    float m[9];

    /**
     * \brief Constructs a Matrix3x3 initialized to the identity matrix.
     */
    Matrix3x3() {
        // Initialize to identity matrix by default
        m[0] = 1; m[1] = 0; m[2] = 0;
        m[3] = 0; m[4] = 1; m[5] = 0;
        m[6] = 0; m[7] = 0; m[8] = 1;
    }

    /**
     * \brief Transforms a 2D point by this matrix (applies translation, rotation, scale).
     * \param vec The 2D vector to transform (treated as a homogeneous point with w=1).
     * \return The transformed 2D vector.
     */
    Vector2D operator*(const Vector2D& vec) const {
        float x = m[0] * vec.x + m[1] * vec.y + m[2];
        float y = m[3] * vec.x + m[4] * vec.y + m[5];
        return Vector2D(x, y);
    }

    /**
     * \brief Creates a 2D rotation matrix for the given angle.
     * \param angle_rad Rotation angle in radians (counter-clockwise positive).
     * \return A Matrix3x3 that rotates vectors by angle_rad.
     */
    static Matrix3x3 CreateRotation(float angle_rad) {
        Matrix3x3 rot;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);

        rot.m[0] = c;  rot.m[1] = -s; rot.m[2] = 0;
        rot.m[3] = s;  rot.m[4] = c;  rot.m[5] = 0;
        rot.m[6] = 0;  rot.m[7] = 0;  rot.m[8] = 1;

        return rot;
    }
};