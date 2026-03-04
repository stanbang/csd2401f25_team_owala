#pragma once
#include "ISystem.h"

namespace Framework {
    class CoreEngine; // Forward declaration

    // Application is just another System that the Engine will run!
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