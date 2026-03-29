/*****************************************************************//**
 * \file   SB_InputSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *      Yar Xuan (20%)
 * 
 * \brief   StageBound specific input system. Contains all testing and debugging keyboard shortcuts.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"

namespace Framework {

    class SB_InputSystem : public ISystem {
    public:
        void Initialize() override {}
        void Update() override;
        void Shutdown() override {}
    };

} // namespace Framework
