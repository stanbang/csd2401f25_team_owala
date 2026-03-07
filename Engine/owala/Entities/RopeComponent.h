// RopeComponent.h
#pragma once
#include "Entity.h"
#include "Vector2D.h"
#include <string>

namespace Framework {
    struct RopeComponent {
        Entity attachedPlatform = 0;
        std::string textureKey = "Rope.png";
        float originalLength = 0.0f;
        float currentLength = 0.0f; 
        Vector2D ceilingAttachmentPoint = { 0.0f, 0.0f };
        Vector2D platformAttachmentPoint = { 0.0f, 0.0f }; 
        bool isActive = true; 
        float ropeWidth = 100.0f;   
        float segmentHeight = 20.0f;   
        int ropeIndex = 0;
        bool hasFoundCeiling = false;

        RopeComponent() = default;
    };
}