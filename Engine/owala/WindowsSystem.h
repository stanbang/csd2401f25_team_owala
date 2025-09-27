/*****************************************************************//**
 * \file   WindowsSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
struct GLFWwindow;

namespace Framework {
    class CoreEngine;

    class WindowsSystem : public ISystem {
    public:
        WindowsSystem(CoreEngine& core, const char* title, int w, int h)
            : core_(&core), title_(title), w_(w), h_(h) {
        }
        void Initialize() override;
        void Update(float) override;
        void Shutdown() override;
        GLFWwindow* window() const { return win_; }

    private:
        CoreEngine* core_ = nullptr;
        const char* title_; int w_, h_;
        GLFWwindow* win_ = nullptr;
    };
} // namespace Framework
