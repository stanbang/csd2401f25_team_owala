/*****************************************************************//**
 * \file   PlayerSystem.cpp
 * \brief
 *
 *
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.h"
#include "AnimationSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes

namespace Framework {

    void AnimationSystem::Update(float dt) {
        // Loop through each entity that with animated texture component
        for (const auto& [entity, tag] : entityStorage_->animatedTex) {
            
        }
    }


    void AnimationSystem::Shutdown() {
        
    }

} // namespace Framework
