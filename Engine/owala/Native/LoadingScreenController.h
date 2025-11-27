/*****************************************************************//**
 * \file   LoadingScreenController.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Declares the LoadingScreenController script to manage loading 
 *          screen behavior.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ScriptableEntity.h"

namespace Framework {

    class LoadingScreenController : public ScriptableEntity {
    protected:
        void OnCreate() override;
        void OnUpdate() override;
        void OnDestroy() override;
    };

}