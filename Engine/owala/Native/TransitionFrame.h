/*****************************************************************//**
 * \file   TransitionFrame.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Declarations for the TransitionFrame script to manage
 *          the static frame sprite that appears during transitions.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ScriptableEntity.h"
#include <string>

namespace Framework {

    class TransitionFrame : public ScriptableEntity {
    public:
        TransitionFrame() = default;
        virtual ~TransitionFrame() = default;

        // Public member to store the original texture key
        std::string frameTextureKey = "";

    protected:
        void OnCreate() override;
        void OnUpdate() override;
        void OnDestroy() override;

    private:
        bool isVisible_ = false;
        
        void UpdateVisibility();
    };
}