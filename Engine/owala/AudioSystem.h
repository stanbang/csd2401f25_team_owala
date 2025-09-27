/*****************************************************************//**
 * \file   AudioSystem.h
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
#include "ISystem.h"
#include "ResourceManager.h"
#include "EntityStorage.h"

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <unordered_map>

namespace Framework {

    class AudioSystem : public ISystem {
    public:
        AudioSystem(EntityStorage& storage);
        ~AudioSystem() override = default;

        // ISystem
        void Initialize() override;
        void Update(float dt) override;
        void Shutdown() override;

        void SetResourceManager(ResourceManager& rcm);

        // ---- simple API ----
        FMOD::System* getFMODSystem() const;

        // Load a sound and keep it cached by id. Returns true on success.
        // loop = true makes it loop; stream = true uses streaming (good for music/large files).
        bool loadSound(const std::string& id,
            const char* path,
            bool loop = false,
            bool stream = false);

        // Play a previously loaded sound by id.
        // Returns true if it started playing.
        bool play(FMOD::Sound& sound, float volume = 1.0f, bool paused = false);

        // Fire-and-forget (load + play once, not cached).
        bool playOneShot(const char* path, float volume = 1.0f);

        // Adjust master/group volume [0..1].
        void setMasterVolume(float volume);

        // Stop all currently playing channels started by this system.
        void stopAll();

        // Unload a cached sound.
        void unloadSound(const std::string& id);

        // tiny helper for FMOD error handling
        static void check(FMOD_RESULT r, const char* where);

    private:

        FMOD::System* system_ = nullptr;
        FMOD::ChannelGroup* masterGroup_ = nullptr;
        ResourceManager* resourceManager_ = nullptr;
		EntityStorage* entityStorage_ = nullptr;

        std::unordered_map<std::string, FMOD::Sound*>   sounds_;
        std::unordered_map<std::string, FMOD::Channel*> channels_; // last channel per id (optional)
    };

} // namespace Framework