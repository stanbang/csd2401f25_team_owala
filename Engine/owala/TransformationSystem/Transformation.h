/*****************************************************************//**
* \file   Transformation.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief    Utility class for creating 2D transformation matrices including
*           rotation, translation, scaling, and combined transformations.
*           Also provides matrix multiplication operator.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Matrix3x3.h"
#include "Vector2D.h"

namespace Framework {
    class Transformation {
    public:
        static Matrix3x3 CreateRotationMatrix(float angleDegrees);
        static Matrix3x3 CreateTranslationMatrix(const Vector2D& translation);
        static Matrix3x3 CreateScaleMatrix(const Vector2D& scale);
        static Matrix3x3 CreateTransformationMatrix(const Vector2D& translation,float rotation, const Vector2D& scale);
    };

    Matrix3x3 operator*(const Matrix3x3& a, const Matrix3x3& b);
}