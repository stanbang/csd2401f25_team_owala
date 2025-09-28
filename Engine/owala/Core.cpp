#include "Core.h"
#include <algorithm>
#include <chrono>

#include "WindowsSystem.h"
#include "InputSystem.h"
#include "RotationSystem.h"
#include "PlayerSystem.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"
#include "ResourceManager.h"
#include "GameManager.h"
#include "Graphics.h"
#include "Editor.h"

namespace Framework {

    // Helper function to get a clean name for each system
    static std::string GetSystemName(const std::type_info& info) {
        if (info == typeid(Framework::WindowsSystem))   return "Windows System";
        if (info == typeid(Framework::InputSystem))     return "Input System";
        if (info == typeid(Framework::RotationSystem))  return "Rotation System";
        if (info == typeid(Framework::PlayerSystem))    return "Player System";
        if (info == typeid(Framework::PhysicsSystem))   return "Physics System";
        if (info == typeid(Framework::AudioSystem))     return "Audio System";
        if (info == typeid(Framework::ResourceManager)) return "Resource Manager";
        if (info == typeid(Framework::GameManager))     return "Game Manager";
        if (info == typeid(Framework::Graphics))        return "Graphics";
        if (info == typeid(Editor::Editor))             return "Editor";
        // Add other systems here
        return "Unknown System"; // if no systems found
    }

    void CoreEngine::Initialize() {
        for (auto* s : systems_) s->Initialize();
        initialized_ = true;
    }

    void CoreEngine::GameLoop() {
        running_ = true;

        using clock = std::chrono::high_resolution_clock;
        std::chrono::time_point<std::chrono::steady_clock> prev = clock::now();

        //Resize the profiler vector once before the loop starts.
        systemTimes_.resize(systems_.size());

        while (running_) {
            auto now = clock::now();
            float dt = std::chrono::duration<float>(now - prev).count();
            prev = now;
            dt = std::min(dt, 0.25f); // protect against giant stalls

            auto frameStartTime = clock::now();

            // --------FOR IMGUI INDIVIDUAL SYSTEM PROFILLING------------ //
            for (size_t i = 0; i < systems_.size(); ++i) {
                auto start = clock::now();
                if (systems_[i]) {
                    systems_[i]->Update(dt);
                }
                auto end = clock::now();

                //Use helper function to get system name
                systemTimes_[i].name = GetSystemName(typeid(*systems_[i]));
                systemTimes_[i].milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
            }

            auto frameEndTime = clock::now();
            totalFrameTime_ = std::chrono::duration<double, std::milli>(frameEndTime - frameStartTime).count();

            // Calculate percentages based on the total frame time
            if (totalFrameTime_ > 0.0) {
                for (auto& timing : systemTimes_) {
                    timing.percentage = static_cast<float>((timing.milliseconds / totalFrameTime_) * 100.0);
                }
            }
        
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
