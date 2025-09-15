#pragma once
/*****************************************************************//**
 * \file   SceneComponents.h
 * \brief  A subset of Components that are specific to scene management
 *
 * \author Jason
 * \date   14 September 2025
 *********************************************************************/
#include <string>

namespace Framework {

    // The scene that this entity belongs to
    struct SceneOwned {
        std::string sceneName;
    };

	// Tag to indicate that this entity should persist across scene changes
    struct PersistentTag {};

    // Tag to indicate that this entity is to be loaded in the scene
    struct ActiveTag {};

} // namespace Framework
