#pragma once
#include <vector>
#include "ISystem.h"

namespace Framework {

    class CoreEngine {
    public:
        CoreEngine() = default;
        CoreEngine(const CoreEngine&) = delete;
        CoreEngine& operator=(const CoreEngine&) = delete;
        ~CoreEngine() = default;

        // Easy-to-read API: caller allocates, Core takes ownership and deletes later
        ISystem* AddSystem(ISystem* system) {
            systems_.push_back(system);
            if (initialized_ && system) system->Initialize(); // hot-add if already running
            return system;
        }

        void Initialize();      // calls Initialize() on all systems (in order)
        void GameLoop();        // variable-timestep loop; calls Update(dt) per system
        void DestroySystems();  // Shutdown() then delete systems (reverse order)

        void RequestQuit() { running_ = false; }

    private:
        bool running_ = false;
        bool initialized_ = false;
        std::vector<ISystem*> systems_;
    };

} // namespace Framework
