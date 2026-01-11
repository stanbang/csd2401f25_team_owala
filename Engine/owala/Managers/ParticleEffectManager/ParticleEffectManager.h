#pragma once
#include <string>
#include <unordered_map>
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

    private:
        ParticleEffectManager() = default;
        std::unordered_map<std::string, ParticleBurstEvent> effectPresets_;
    };
}