#include "ParticleEffectManager.h"
#include "FileUtility.h"
#include "Logger.h"
#include "rapidjson/document.h"

namespace Framework {
    bool ParticleEffectManager::LoadEffectPresets(const std::string& filepath) {
        rapidjson::Document doc;
        if (!FileIOSystem::ReadFile(filepath, doc)) {
            Logger::Get().Error(DebugType::ResourceManager) 
                << "Failed to load particle effect presets: " << filepath << std::endl;
            return false;
        }

        if (!doc.IsObject()) {
            Logger::Get().Error(DebugType::ResourceManager)
                << "Particle effect presets file is not a valid JSON object: " << filepath << std::endl;
            return false;
        }

        effectPresets_.clear();
        triggerToEffects_.clear();
        effectToTrigger_.clear();

        int presetsLoaded = 0;
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
            std::string effectName = it->name.GetString();
            const auto& effectObj = it->value;

            if (!effectObj.IsObject()) {
                Logger::Get().Error(DebugType::ResourceManager)
                    << "Particle effect preset '" << effectName << "' is not a valid object, skipping." << std::endl;
                continue;
            }

            ParticleBurstEvent effect;
            effect.position = Vector2D{ 0.0f, 0.0f };

            // Required fields
            if (effectObj.HasMember("textureKey") && effectObj["textureKey"].IsString()) {
                effect.textureKey = effectObj["textureKey"].GetString();
            } else {
                Logger::Get().Error(DebugType::ResourceManager)
                    << "Particle effect preset '" << effectName << "' missing required 'textureKey' field." << std::endl;
                continue;
            }

            // Optional fields with defaults
            if (effectObj.HasMember("isAnimated") && effectObj["isAnimated"].IsBool()) {
                effect.isAnimated = effectObj["isAnimated"].GetBool();
            }
            if (effectObj.HasMember("lifetime") && effectObj["lifetime"].IsNumber()) {
                effect.lifetime = effectObj["lifetime"].GetFloat();
            }
            if (effectObj.HasMember("count") && effectObj["count"].IsInt()) {
                effect.count = effectObj["count"].GetInt();
            }
            if (effectObj.HasMember("speed") && effectObj["speed"].IsNumber()) {
                effect.speed = effectObj["speed"].GetFloat();
            }
            if (effectObj.HasMember("isScreenSpace") && effectObj["isScreenSpace"].IsBool()) {
                effect.isScreenSpace = effectObj["isScreenSpace"].GetBool();
            }

            // Size
            if (effectObj.HasMember("size") && effectObj["size"].IsObject()) {
                const auto& sizeObj = effectObj["size"];
                effect.size.x = (sizeObj.HasMember("x") && sizeObj["x"].IsNumber()) ? sizeObj["x"].GetFloat() : 0.1f;
                effect.size.y = (sizeObj.HasMember("y") && sizeObj["y"].IsNumber()) ? sizeObj["y"].GetFloat() : 0.1f;
            }

            // Offset
            if (effectObj.HasMember("offset") && effectObj["offset"].IsObject()) {
                const auto& offsetObj = effectObj["offset"];
                effect.offset.x = (offsetObj.HasMember("x") && offsetObj["x"].IsNumber()) ? offsetObj["x"].GetFloat() : 0.0f;
                effect.offset.y = (offsetObj.HasMember("y") && offsetObj["y"].IsNumber()) ? offsetObj["y"].GetFloat() : 0.0f;
            }

            // Style (convert string to enum)
            if (effectObj.HasMember("style") && effectObj["style"].IsString()) {
                std::string styleStr = effectObj["style"].GetString();
                if (styleStr == "FROM_POINT") {
                    effect.style = EmissionStyle::FROM_POINT;
                } else if (styleStr == "FROM_RADIUS") {
                    effect.style = EmissionStyle::FROM_RADIUS;
                } else if (styleStr == "FROM_TOP_VIEWPORT") {
                    effect.style = EmissionStyle::FROM_TOP_VIEWPORT;
                } else if (styleStr == "FROM_LEFT_VIEWPORT") {
                    effect.style = EmissionStyle::FROM_LEFT_VIEWPORT;
                } else if (styleStr == "FROM_RIGHT_VIEWPORT") {
                    effect.style = EmissionStyle::FROM_RIGHT_VIEWPORT;
                } else if (styleStr == "FROM_BOTTOM_VIEWPORT") {
                    effect.style = EmissionStyle::FROM_BOTTOM_VIEWPORT;
                } else if (styleStr == "FROM_LINE") {
                    effect.style = EmissionStyle::FROM_LINE;
                } else if (styleStr == "FROM_CIRCLE_EDGE") {
                    effect.style = EmissionStyle::FROM_CIRCLE_EDGE;
                }
            }

            // Effect type (convert string to enum)
            if (effectObj.HasMember("effect") && effectObj["effect"].IsString()) {
                std::string effectTypeStr = effectObj["effect"].GetString();
                if (effectTypeStr == "DRIFT") {
                    effect.effect = EffectType::DRIFT;
                } else if (effectTypeStr == "RANDOM") {
                    effect.effect = EffectType::RANDOM;
                } else {
                    effect.effect = EffectType::NONE;
                }
            }

            if (effectObj.HasMember("effectStrength") && effectObj["effectStrength"].IsNumber()) {
                effect.effectStrength = effectObj["effectStrength"].GetFloat();
            }

            if (effectObj.HasMember("radius") && effectObj["radius"].IsNumber()) {
                effect.radius = effectObj["radius"].GetFloat();
            }

            // Load trigger event name
            std::string triggerEvent;
            if (effectObj.HasMember("triggerEvent") && effectObj["triggerEvent"].IsString()) {
                triggerEvent = effectObj["triggerEvent"].GetString();
                effectToTrigger_[effectName] = triggerEvent;
                triggerToEffects_[triggerEvent].push_back(effectName);
                
                Logger::Get().Info(DebugType::ResourceManager)
                    << "Loaded particle effect '" << effectName << "' triggered by '" << triggerEvent << "'" << std::endl;
            } else {
                Logger::Get().Info(DebugType::ResourceManager)
                    << "Loaded particle effect '" << effectName << "' (no trigger)" << std::endl;
            }

            effectPresets_[effectName] = effect;
            presetsLoaded++;
        }

        Logger::Get().Info(DebugType::ResourceManager) 
            << "Loaded " << presetsLoaded << " particle effect presets from: " << filepath << std::endl;
        
        // Log all trigger mappings
        for (const auto& pair : triggerToEffects_) {
            Logger::Get().Info(DebugType::ResourceManager)
                << "Trigger '" << pair.first << "' -> " << pair.second.size() << " effects" << std::endl;
        }
        
        return presetsLoaded > 0;
    }

    bool ParticleEffectManager::GetEffectPreset(const std::string& effectName, ParticleBurstEvent& outEvent) {
        auto it = effectPresets_.find(effectName);
        if (it == effectPresets_.end()) {
            Logger::Get().Error(DebugType::ResourceManager) 
                << "Particle effect preset not found: " << effectName << std::endl;
            return false;
        }

        outEvent = it->second;
        return true;
    }

    std::vector<std::string> ParticleEffectManager::GetEffectsByTrigger(const std::string& triggerEventName) const {
        auto it = triggerToEffects_.find(triggerEventName);
        if (it != triggerToEffects_.end()) {
            return it->second;
        }
        return std::vector<std::string>();
    }

    std::string ParticleEffectManager::GetTriggerEvent(const std::string& effectName) const {
        auto it = effectToTrigger_.find(effectName);
        if (it != effectToTrigger_.end()) {
            return it->second;
        }
        return "";
    }
}