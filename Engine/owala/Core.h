/*****************************************************************//**
 * \file   Core.h
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
#include <vector>
#include "ISystem.h"
#include <string>

namespace Framework {
    //structure to hold performance data for a single system for system performance data
    struct SystemTime {
        std::string name;
        double milliseconds = 0.0;
        float percentage = 0.0f;
    };

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

        // Public getter so GameManager and ImGui can access the timing data
        const std::vector<SystemTime>& GetSystemTimes() const { return systemTimes_; }
        double GetTotalFrameTime() const { return totalFrameTime_; }

        void Initialize();      // calls Initialize() on all systems (in order)
        void GameLoop();        // variable-timestep loop; calls Update(dt) per system
        void DestroySystems();  // Shutdown() then delete systems (reverse order)

        void RequestQuit() { running_ = false; }

    private:
        bool running_ = false;
        bool initialized_ = false;
        std::vector<ISystem*> systems_;

        // Member variables for storing individual system time results
        std::vector<SystemTime> systemTimes_;
        double totalFrameTime_ = 0.0;
    };

} // namespace Framework
