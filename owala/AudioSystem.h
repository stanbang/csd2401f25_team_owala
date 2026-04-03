/*****************************************************************//**
 * \file   AudioSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (75%)
 * \secondary_author
 *      Yar Xuan  (15%)
 *      Ng Jing Wen (10%)
 *
 * \brief
 *   Audio system wrapper around FMOD for loading, playing, and managing
 *   sounds within the Framework ECS. Provides simple caching by ID,
 *   fire-and-forget playback, master volume control, bulk stop,
 *   per-channel-group reverb (DSP), and basic 2D->3D spatial audio.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "ResourceManager.h"
#include "EntityStorage.h"
#include "GameMessages.h"
#include "MessageBus.h"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_dsp.h>   // DSP parameter enums live here

#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <cstdio>
#include <algorithm>

namespace Framework {

    class AudioSystem : public ISystem {
    public:
        AudioSystem() = default;
        ~AudioSystem() override = default;

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        FMOD::System* getFMODSystem() const;

        // ---------------- Loading ----------------
        // Added: is3D flag (default false). If true, creates FMOD_3D sound.
        bool loadSound(const std::string& id,
            const char* path,
            bool loop = false,
            bool stream = false,
            bool is3D = false);

        // ---------------- Playback ----------------
        FMOD::Channel* play(FMOD::Sound& sound, AudioType type, float volume = 1.0f, bool paused = false);

        // Added: 3D playback (2D world -> XZ plane).
        // Sound should be created with FMOD_3D for best results, but channel mode is forced too.
        FMOD::Channel* play3D(FMOD::Sound& sound, AudioType type, float volume,
            const Vector2D& worldPos,
            float minDistance = 50.0f, float maxDistance = 700.0f,
            bool paused = false);

        bool playOneShot(const char* path, float volume = 1.0f);

        void setMasterVolume(float volume);
        void setMusicVolume(float volume);
        void setSFXVolume(float volume);

        float getMasterVolume();
        float getMusicVolume();
        float getSFXVolume();
        
        void Stop2DAudio(Entity entity, float fadeDuration = 0.f);
        void Stop3DAudio(Entity entity, float fadeDuration = 0.f);
        void StopAudio(Entity entity, float fadeDuration = 0.f);
        void stopAll(bool fadeOut = true);
        void unloadSound(const std::string& id);

        static void check(FMOD_RESULT r, const char* where);

        void OnButtonClicked(const ButtonClickedEvent& event);

        // Interruption handling
        void SuspendAudio();
        void ResumeAudio();

        // Random SFX helpers
        std::string getRandomSound(std::string eventKey);

        FMOD::Channel* PlayRandomSound(std::string eventKey, AudioType audiotype, float volume, bool startPaused = false);
        FMOD::Channel* PlayRandomSound3D(std::string eventKey, AudioType audiotype, float volume, Vector2D position, float minDist, float maxDist, bool startPaused = false);

        // Fade helper
        void FadeOutChannel(FMOD::Channel* channel, float duration);

        // ---------------- Reverb (per ChannelGroup) ----------------
        void EnableGroupReverb(AudioType type, bool enable);
        void SetGroupReverbWetDb(AudioType type, float wetDb);   // e.g. -18..0 dB
        void SetGroupReverbRoom(AudioType type, float room01);   // 0..1 (mapped to decay/density/diffusion)
        void SetGroupReverbDecay(AudioType type, float decayMs); // milliseconds

        // ---------------- Spatial audio ----------------
        void Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale);

    private:
        void UpdateListenerFromPlayer_();
        void Update3DChannelPositions_();

        // Tracks channels fading out: <Channel Pointer, Fade Speed (vol/sec)>
        std::unordered_map<FMOD::Channel*, float> fadingChannels_;

        // FMOD Core pointers
        FMOD::System* system_ = nullptr;
        FMOD::ChannelGroup* masterGroup_ = nullptr;
        FMOD::ChannelGroup* musicGroup_ = nullptr;
        FMOD::ChannelGroup* sfxGroup_ = nullptr;
        FMOD::ChannelGroup* drySfxGroup_ = nullptr;

        // Per-group reverb DSPs
        FMOD::DSP* musicReverbDSP_ = nullptr;
        FMOD::DSP* sfxReverbDSP_ = nullptr;

        // Caches
        std::unordered_map<std::string, FMOD::Sound*>   sounds_;
        std::unordered_map<std::string, FMOD::Channel*> channels_;
        std::unordered_map<std::string, std::vector<std::string>> soundPaths_;
        std::map<Entity, FMOD::Channel*> entityChannels_;    // AudioComponent-driven 2D channels
        std::map<Entity, FMOD::Channel*> entity3DChannels_;  // PlaySound3DEvent-driven 3D channels (position-tracked)

        // Spatial audio tunables
        float distanceFactor_ = 1.0f;
        float minHearingRange = 50.0f;
        float maxHearingRange = 700.0f;

        // Data-driven animation event to audio mapping (loaded from animationEventAudio.json)
        std::unordered_map<std::string, AnimationEventAudioConfig> animationEventAudioMap_;

        // Track the last played sound index for each event key
        std::unordered_map<std::string, int> lastPlayedIndices_;
    };

} // namespace Framework