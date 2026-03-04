/*****************************************************************//**
 * \file  ParticleEffectManager.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Declaration of the Particle Effect Manager class. Which
 *          loads particle effect presets from JSON files and manages
 *          the mappings between trigger events and effect presets.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "GameMessages.h"

namespace Framework {
    class ParticleEffectManager {
    public:
        static ParticleEffectManager& GetInstance() {
            static ParticleEffectManager instance;
            return instance;
        }

        // Load presets from JSON file
        bool LoadEffectPresets(const std::string& filepath);

        // Get a preset and apply it to an event
        bool GetEffectPreset(const std::string& effectName, ParticleBurstEvent& outEvent);

        // Get all effect names that are triggered by a specific event
        std::vector<std::string> GetEffectsByTrigger(const std::string& triggerEventName) const;
        
        // Get the trigger event name for an effect preset
        std::string GetTriggerEvent(const std::string& effectName) const;

    private:
        ParticleEffectManager() = default;
        std::unordered_map<std::string, ParticleBurstEvent> effectPresets_;
        
        // Map trigger event names to effect preset names
        // e.g., "WorldRotationEvent" -> ["DustVFX", "RockDebrisVFX"]
        std::unordered_map<std::string, std::vector<std::string>> triggerToEffects_;
        
        // Map effect names to their trigger event names
        std::unordered_map<std::string, std::string> effectToTrigger_;
    };
}