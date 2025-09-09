#include "Core.h"
#include <algorithm>
#include <chrono>

namespace Framework {

    void CoreEngine::Initialize() {
        for (auto* s : systems_) s->Initialize();
        initialized_ = true;
    }

    void CoreEngine::GameLoop() {
        running_ = true;

        using clock = std::chrono::high_resolution_clock;
        auto prev = clock::now();

        while (running_) {
            auto now = clock::now();
            float dt = std::chrono::duration<float>(now - prev).count();
            prev = now;
            dt = std::min(dt, 0.25f); // protect against giant stalls

            for (auto* s : systems_) s->Update(dt);
        }
    }

    void CoreEngine::DestroySystems() {
        // shutdown in reverse add order, then delete
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            if (*it) (*it)->Shutdown();
        }
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            delete* it;
        }
        systems_.clear();
        initialized_ = false;
    }

} // namespace Framework
