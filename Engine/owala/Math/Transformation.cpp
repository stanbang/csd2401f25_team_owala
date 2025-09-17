// Transformation.cpp
#include "Transformation.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
namespace Framework {
    Matrix3x3 Transformation::CreateRotationMatrix(float angleDegrees) {
        float angleRad = angleDegrees * M_PI / 180.0f;
        return Matrix3x3::CreateRotation(angleRad);
    }

    Matrix3x3 Transformation::CreateTranslationMatrix(const Vector2D& translation) {
        Matrix3x3 trans; 
        trans.m[2] = translation.x;  
        trans.m[5] = translation.y;  
        return trans;
    }

  
    Matrix3x3 Transformation::CreateScaleMatrix(const Vector2D& scale) {
        Matrix3x3 scaleMat; 
        scaleMat.m[0] = scale.x;  
        scaleMat.m[4] = scale.y;  
        return scaleMat;
    }

    Matrix3x3 Transformation::CreateTransformationMatrix( const Vector2D& translation, float rotation, const Vector2D& scale) {
        //  Scale -> Rotate -> Translate
        Matrix3x3 scaleMat = CreateScaleMatrix(scale);
        Matrix3x3 rotationMat = CreateRotationMatrix(rotation);
        Matrix3x3 translationMat = CreateTranslationMatrix(translation);

        // T * R * S
        return translationMat * (rotationMat * scaleMat);
    }

    Matrix3x3 operator*(const Matrix3x3& a, const Matrix3x3& b) {
        Matrix3x3 result;

        // Row 0
        result.m[0] = a.m[0] * b.m[0] + a.m[1] * b.m[3] + a.m[2] * b.m[6];
        result.m[1] = a.m[0] * b.m[1] + a.m[1] * b.m[4] + a.m[2] * b.m[7];
        result.m[2] = a.m[0] * b.m[2] + a.m[1] * b.m[5] + a.m[2] * b.m[8];

        // Row 1
        result.m[3] = a.m[3] * b.m[0] + a.m[4] * b.m[3] + a.m[5] * b.m[6];
        result.m[4] = a.m[3] * b.m[1] + a.m[4] * b.m[4] + a.m[5] * b.m[7];
        result.m[5] = a.m[3] * b.m[2] + a.m[4] * b.m[5] + a.m[5] * b.m[8];

        // Row 2
        result.m[6] = a.m[6] * b.m[0] + a.m[7] * b.m[3] + a.m[8] * b.m[6];
        result.m[7] = a.m[6] * b.m[1] + a.m[7] * b.m[4] + a.m[8] * b.m[7];
        result.m[8] = a.m[6] * b.m[2] + a.m[7] * b.m[5] + a.m[8] * b.m[8];

        return result;
    }
}