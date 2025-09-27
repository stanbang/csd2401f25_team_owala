/*****************************************************************//**
 * \file   PlayerSystem.cpp
 * \brief
 *
 *
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.h"
#include "EntityManager.h"
#include "AnimationSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes

namespace Framework {

    void AnimationSystem::Update(float dt) {
		EntityManager entityMgr(*entityStorage_);
        // Loop through each entity that with animated texture component
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(AnimatedTextureComponentType)) {
			const AnimatedTextureComponent& tag = entityMgr.GetComponent<AnimatedTextureComponent>(entity);
        }
    }


    void AnimationSystem::Shutdown() {
        
    }

} // namespace Framework
