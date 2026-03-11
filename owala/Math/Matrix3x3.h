/*****************************************************************//**
* \file   Matrix3x3.h
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief Math Library For all 2x2 Matrix Functions
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

    Matrix3x3() {
        // Initialize to identity matrix by default
        m[0] = 1; m[1] = 0; m[2] = 0;
        m[3] = 0; m[4] = 1; m[5] = 0;
        m[6] = 0; m[7] = 0; m[8] = 1;
    }

    // Matrix-vector multiplication (for transforming points)
    Vector2D operator*(const Vector2D& vec) const {
        float x = m[0] * vec.x + m[1] * vec.y + m[2];
        float y = m[3] * vec.x + m[4] * vec.y + m[5];
        return Vector2D(x, y);
    }

    // Static function to create a rotation matrix
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