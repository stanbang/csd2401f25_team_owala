#pragma once
#include "Graphics.h"
#include <vector>

namespace Framework {

    // One drawable sprite (texture + size + tint + optional rotation)
    struct Sprite {
        Texture tex = 0;                 // OpenGL texture handle
        Vec2    halfSize{ 0.2f, 0.2f };   // NDC half-size (0.2 ? 40% of screen height)
        float   rotation = 0.0f;         // radians
        Color   tint{ 1.0f, 1.0f, 1.0f, 1.0f }; // multiply color
    };

    // A set of sprite frames (e.g., idle/walk frames); current selects which to draw
    struct Sprites {
        std::vector<Sprite> frames;
        int                 current = 0;
    };

} // namespace Framework
