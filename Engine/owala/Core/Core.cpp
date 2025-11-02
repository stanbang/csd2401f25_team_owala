/*****************************************************************//**
 * \file   Core.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *     Lur Pei Hao Stanley (20%)
 *
 * \brief
 *   Core engine loop and system manager.
 *   Handles initialization, per-frame updates, shutdown, and performance timing
 *   of subsystems derived from ISystem, with Editor/Play mode support.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "Core.h"
#include <algorithm>
#include <chrono>

#include "WindowsSystem.h"
#include "InputSystem.h"
#include "TransformationSystem.h"
#include "PlayerSystem.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"
#include "ResourceManager.h"
#include "GameManager.h"
#include "Graphics.h"
#include "OwalaTime.h"
#ifdef BUILD_EDITOR
#include "Editor.h"
#endif


namespace Framework {

    // ------------------------------------------------------------------------
    // Helper to return clean display names for profiling UI
    // ------------------------------------------------------------------------
    static std::string GetSystemName(const std::type_info& info) {
        if (info == typeid(Framework::WindowsSystem))   return "Windows System";
        if (info == typeid(Framework::InputSystem))     return "Input System";
        if (info == typeid(Framework::TransformationSystem))  return "Transformation System";
        if (info == typeid(Framework::PlayerSystem))    return "Player System";
        if (info == typeid(Framework::PhysicsSystem))   return "Physics System";
        if (info == typeid(Framework::AudioSystem))     return "Audio System";
        if (info == typeid(Framework::ResourceManager)) return "Resource Manager";
        if (info == typeid(Framework::GameManager))     return "Game Manager";
        if (info == typeid(Framework::Graphics))        return "Graphics";
        if (info == typeid(Framework::AISystem))         return "AI System";
	#ifdef BUILD_EDITOR
        if (info == typeid(Editor::Editor))             return "Editor";
    #endif
        // Add other systems here
        return "Unknown System"; // if no systems found
    }

    // ------------------------------------------------------------------------
    // Add systems
    // ------------------------------------------------------------------------
    ISystem* CoreEngine::AddSystem(ISystem* system) {
        systems_.push_back(system);
        if (initialized_ && system)
            system->Initialize(); // hot-add
        return system;
    }

    // ------------------------------------------------------------------------
    // Initialize all systems
    // ------------------------------------------------------------------------
    void CoreEngine::Initialize() {
        for (auto* s : systems_)
            if (s) s->Initialize();
        initialized_ = true;
    }

    // ------------------------------------------------------------------------
    // Main game loop with per-system profiling and mode filtering
    // ------------------------------------------------------------------------
    void CoreEngine::GameLoop() {
        running_ = true;

        using clock = std::chrono::high_resolution_clock;
        auto prev = clock::now();
        Framework::Time::Initialize();
        systemTimes_.resize(systems_.size());

        while (running_) {
            // --- Time step calculation ---
            auto now = clock::now();
            Framework::Time::BeginFrame();
            float dt = std::chrono::duration<float>(now - prev).count();
            prev = now;
            dt = std::min(dt, 0.25f); // cap huge delta time

            auto frameStart = clock::now();

            // --- Per-system updates with profiling ---
            for (size_t i = 0; i < systems_.size(); ++i) {
                ISystem* sys = systems_[i];
                if (!sys) continue;

                auto start = clock::now();
                std::string sysName = GetSystemName(typeid(*sys));
                bool shouldUpdate = true;

                // --- Mode-based filtering ---
                if (mode_ == EngineMode::Editor) {
                    // Skip gameplay systems in Editor mode
                    if (sysName == "Player System" ||
                        sysName == "Physics System" ||
                        sysName == "Rotation System" ||
                        sysName == "Audio System") {
                        shouldUpdate = false;
                    }
                }
                // Always update Editor and Graphics for overlay drawing
                if (sysName == "Editor" || sysName == "Graphics")
                    shouldUpdate = true;

                // --- Execute ---
                if (shouldUpdate)
                    sys->Update();

                auto end = clock::now();
                systemTimes_[i].name = sysName;
                systemTimes_[i].milliseconds =
                    std::chrono::duration<double, std::milli>(end - start).count();
            }

            // --- Compute profiling summary ---
            auto frameEnd = clock::now();
            totalFrameTime_ = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

            if (totalFrameTime_ > 0.0) {
                for (auto& t : systemTimes_)
                    t.percentage = static_cast<float>((t.milliseconds / totalFrameTime_) * 100.0);
            }
        }

		Framework::Time::EndFrame();
    }

    // ------------------------------------------------------------------------
    // Shutdown and cleanup
    // ------------------------------------------------------------------------
    void CoreEngine::DestroySystems() {
        // Shutdown in reverse order
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            if (*it) (*it)->Shutdown();
        }

        // Delete in reverse order
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            delete* it;
        }

        systems_.clear();
        initialized_ = false;
    }

} // namespace Framework
