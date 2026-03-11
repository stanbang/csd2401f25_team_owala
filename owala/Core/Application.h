/*****************************************************************//**
* \file   Application.h
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief    This file declares the Application class, which is a user-defined 
*           system that the CoreEngine will run. It also declares the CreateApplication 
*           function, which is defined in the user's game project and returns an instance 
*           of their Application subclass.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"

namespace Framework {
    class CoreEngine; // Forward declaration

    // Application is just another System that the Engine will run
    class Application : public ISystem {
    public:
        virtual ~Application() = default;

        // ISystem requires these:
        // virtual void Initialize() = 0;
        // virtual void Update() = 0;
        // virtual void Shutdown() = 0;
    };

    // This function is declared in the engine, but DEFINED in your Game project
    Application* CreateApplication(CoreEngine& engine);
}