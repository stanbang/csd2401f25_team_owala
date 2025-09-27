/*****************************************************************//**
* \file   SceneComponents.h
*
* \primary_author
*      Jason Lou
* \secondary_author
*
*
* \brief	A subset of Components that are specific to scene management.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
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
