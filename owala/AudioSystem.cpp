/*****************************************************************//**
 * \file   AudioSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *      Yar Xuan  (20%)
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

#include "pch.h"
#include "AudioSystem.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"

namespace Framework {

    void AudioSystem::check(FMOD_RESULT r, const char* where) {
        if (r != FMOD_OK) {
            std::fprintf(stderr, "[FMOD] %s failed: (%d) %s\n",
                where, (int)r, FMOD_ErrorString(r));
        }
    }

    FMOD::System* AudioSystem::getFMODSystem() const { return system_; }

    void AudioSystem::Initialize() {
        FMOD_RESULT r;

        r = FMOD::System_Create(&system_);
        check(r, "System_Create");

        // Up to 512 channels is plenty for a small project
        r = system_->init(512, FMOD_INIT_NORMAL, nullptr);
        check(r, "System::init");

        // Optional: get a master channel group to control overall volume
        r = system_->getMasterChannelGroup(&masterGroup_);
        check(r, "System::getMasterChannelGroup");

        // Sub groups to separate music and SFX
        r = system_->createChannelGroup("Music", &musicGroup_);
        check(r, "Create Music Group");
        r = system_->createChannelGroup("SFX", &sfxGroup_);
        check(r, "Create SFX Group");

        // This ensures Master Volume affects Music and SFX
        if (masterGroup_ && musicGroup_) masterGroup_->addGroup(musicGroup_);
        if (masterGroup_ && sfxGroup_)   masterGroup_->addGroup(sfxGroup_);

        // ---------------- Spatial audio baseline ----------------
        // 2D game mapped to XZ plane.
        distanceFactor_ = 1.0f;
        system_->set3DSettings(1.0f, distanceFactor_, 1.0f);

        // ---------------- Per-group Reverb DSP ----------------
        // SFX reverb DSP
        {
            r = system_->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &sfxReverbDSP_);
            check(r, "createDSPByType(SFXREVERB)");

            if (sfxReverbDSP_ && sfxGroup_) {
                r = sfxGroup_->addDSP(0, sfxReverbDSP_);
                check(r, "sfxGroup_->addDSP(reverb)");

                // Starter preset (subtle room)
                sfxReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, -12.0f);     // dB
                sfxReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 0.0f);      // dB
                sfxReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, 900.0f);   // ms
                sfxReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, 0.6f);       // 0..1
                sfxReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, 0.7f);     // 0..1
                sfxReverbDSP_->setBypass(true); // off by default
            }
        }

        // Music reverb DSP (optional)
        {
            r = system_->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &musicReverbDSP_);
            check(r, "createDSPByType(MUSIC SFXREVERB)");

            if (musicReverbDSP_ && musicGroup_) {
                r = musicGroup_->addDSP(0, musicReverbDSP_);
                check(r, "musicGroup_->addDSP(reverb)");

                // Starter preset (lighter)
                musicReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, -18.0f);    // dB
                musicReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 0.0f);     // dB
                musicReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, 1200.0f); // ms
                musicReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, 0.4f);      // 0..1
                musicReverbDSP_->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, 0.5f);    // 0..1
                musicReverbDSP_->setBypass(true); // off by default
            }
        }

        // Load in audio file names
        FileIOSystem::LoadAudioFileNames(soundPaths_);

        // Subscribe to Player Events
        MessageBus::Instance().Subscribe<PlayerJumpedEvent>([this](const PlayerJumpedEvent&) {this->PlayRandomSound("Player_Jump", AudioType::SFX, 0.3f); });
        MessageBus::Instance().Subscribe<PlayerLandEvent>([this](const PlayerLandEvent&) {this->PlayRandomSound("Player_Land", AudioType::SFX, 0.3f);  });

        MessageBus::Instance().Subscribe<PlayerDamagedEvent>([this](const PlayerDamagedEvent&) { this->PlayRandomSound("Player_Hurt", AudioType::SFX, 0.3f); });
        MessageBus::Instance().Subscribe<PlayerDeathEvent>([this](const PlayerDeathEvent& e) {
            if (!e.deathAnimationFinished) {  // Only play sound on the initial death, not after animation ends
                this->PlayRandomSound("Player_Death", AudioType::SFX, 0.3f);
            }
            });

		// Subscribe to Animation Events (for frame-specific SFX)
        MessageBus::Instance().Subscribe<AnimationEvent>([this](const AnimationEvent& e) {
            // Filter for the events this system cares about
            if (e.eventName == "Footstep") {
                this->PlayRandomSound("Player_Footstep", AudioType::SFX, 0.5f);
            }
            else if (e.eventName == "Bird_Fly") {
                EntityManager entityMgr(GameManager::GetInstance().GetECS());
                Vector2D position{};
                if (entityMgr.HasComponent(e.entity, TransformType)) {
                    position = entityMgr.GetComponent<Transform>(e.entity).position;
                }
                else return;

                this->PlayRandomSound3D("Bird_Fly", AudioType::SFX, 0.3f, position, minHearingRange, maxHearingRange);
            }
            });

        // Subscribe to Bird Enemy Events
        MessageBus::Instance().Subscribe<BirdWakeEvent>([this](const BirdWakeEvent& e) {
            this->PlayRandomSound3D("Bird_Wake", AudioType::SFX, 0.3f, e.position, minHearingRange, maxHearingRange); 
        });
        MessageBus::Instance().Subscribe<BirdAttackEvent>([this](const BirdAttackEvent& e) {
            this->PlayRandomSound3D("Bird_Attack", AudioType::SFX, 0.3f, e.position, minHearingRange, maxHearingRange); 
        });

        // Subscribe to World Rotation Event
        MessageBus::Instance().Subscribe<WorldRotationEvent>([this](const WorldRotationEvent&) {this->PlayRandomSound("Stage_Rotate", AudioType::SFX, 1.0f); });
        // Subscribe to Trap Platform Land Event
        MessageBus::Instance().Subscribe<TrapPlatformLandEvent>([this](const TrapPlatformLandEvent& e) {
            this->PlayRandomSound3D("Platform_Land", AudioType::SFX, 1.0f, e.position, minHearingRange, maxHearingRange);
        });
        // Subscribe to Curtains Open/Close Event
        MessageBus::Instance().Subscribe<CurtainsOpenEvent>([this](const CurtainsOpenEvent&) {this->PlayRandomSound("Curtains_Open", AudioType::SFX, 1.0f); });
        MessageBus::Instance().Subscribe<CurtainsCloseEvent>([this](const CurtainsCloseEvent&) {this->PlayRandomSound("Curtains_Close", AudioType::SFX, 1.0f); });

        // Subscribe to ButtonClickedEvent
        MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& e) {this->OnButtonClicked(e); });
        

        // ===== DEBUG: turn on SFX reverb by default =====
        EnableGroupReverb(AudioType::SFX, true);
        SetGroupReverbWetDb(AudioType::SFX, -7.0f);
        SetGroupReverbRoom(AudioType::SFX, 0.8f);
     }

    void AudioSystem::UpdateListenerFromPlayer_() {
        if (!system_) return;

        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        std::vector<Entity> playerEntities = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
        if (playerEntities.empty()) return;

        Entity playerID = playerEntities[0];
        Vector2D p2 = entityMgr.GetComponent<Transform>(playerID).position;

        // Map your 2D (x,y) to FMOD 3D (x,z). Keep y=0.
        FMOD_VECTOR pos{ p2.x, 0.0f, p2.y };
        FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };

        FMOD_VECTOR forward{ 0.0f, 0.0f, 1.0f };
        FMOD_VECTOR up{ 0.0f, 1.0f, 0.0f };

        system_->set3DListenerAttributes(0, &pos, &vel, &forward, &up);
    }

    void AudioSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        ResourceManager& resourceManager_ = *gm.GetSystems().resourceManager_;
        EntityManager entityMgr(gm.GetECS());

        // --- Process Fading Channels ---
        float dt = Time::DeltaTime();

        for (auto it = fadingChannels_.begin(); it != fadingChannels_.end(); ) {
            FMOD::Channel* ch = it->first;
            float fadeSpeed = it->second;

            bool isPlaying = false;
            if (ch) ch->isPlaying(&isPlaying);

            if (!ch || !isPlaying) {
                it = fadingChannels_.erase(it); // Channel died naturally
                continue;
            }

            float vol = 0.0f;
            ch->getVolume(&vol);
            vol -= fadeSpeed * dt; // Reduce volume

            if (vol <= 0.0f) {
                ch->stop(); // Done fading
                it = fadingChannels_.erase(it);
            }
            else {
                ch->setVolume(vol); // Apply new volume
                ++it;
            }
        }

        // --- Update Listener for Spatial Audio ---
        UpdateListenerFromPlayer_();

        // --- Update FMOD System ---
        if (system_) {
            FMOD_RESULT r = system_->update();
            check(r, "System::update");
        }

        // --- Manage Entity Channels ---
        for (auto it = entityChannels_.begin(); it != entityChannels_.end(); ) {
            Entity ownerEntity = it->first;
            FMOD::Channel* channel = it->second;

            // Check if the entity still "Exists" in the scene logic
            bool isEntityActive = entityMgr.HasComponent(ownerEntity, LoadedTagType) && entityMgr.IsActive(ownerEntity);

            // Check if the channel is actually still playing
            bool isChannelPlaying = false;
            if (channel) {
                channel->isPlaying(&isChannelPlaying);
            }

            if (!isChannelPlaying) {
                // Sound finished naturally: just remove, no fade needed
                it = entityChannels_.erase(it);
            }
            else if (!isEntityActive) {
                // Entity disabled/died: Force fade out (e.g., 1.0 second)
                FadeOutChannel(channel, 1.0f);
                it = entityChannels_.erase(it); // Detach from entity map
            }
            else {
                ++it;
            }
        }

        // --- Start New Sounds ---
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, AudioComponentType)) {

            // Check if Entity is active in the scene. Loads if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
            // Check if entity is tied to a button
            if (entityMgr.HasComponent(entity, ButtonType)) {
                continue; //Skip button entities, handle separately.
            }
            // Check if entity has AudioComponent
            if (!entityMgr.HasComponent(entity, AudioComponentType)) continue;

            AudioComponent& comp = entityMgr.GetComponent<AudioComponent>(entity);
            if (comp.soundKey.empty()) continue;

            try {
                FMOD::Sound& sound = resourceManager_.get<FMOD::Sound>(comp.soundKey);

                // check if sound is already playing
                bool isPlaying = false;
                auto it = channels_.find(comp.soundKey);
                if (it != channels_.end() && it->second) {
                    it->second->isPlaying(&isPlaying);
                }

                // play if not playing
                if (!isPlaying) {
                    FMOD::Channel* newChannel = this->play(sound, comp.audioType, comp.volume, false);

                    // Save to the map so the next frame knows it is playing
                    if (newChannel) {
                        channels_[comp.soundKey] = newChannel;
                        entityChannels_[entity] = newChannel;
                    }
                }

            }
            catch (const GameException& e) {
                (void)e; //Suppress unused variable warning
                std::fprintf(stderr, "[Audio] Sound '%s' not found in ResourceManager\n", comp.soundKey.c_str());
            }
        }
    }

    void AudioSystem::Shutdown() {
        // Stop and release channels
        stopAll();

        // Release DSPs first
        if (sfxReverbDSP_) { sfxReverbDSP_->release();   sfxReverbDSP_ = nullptr; }
        if (musicReverbDSP_) { musicReverbDSP_->release(); musicReverbDSP_ = nullptr; }

        // Release all cached sounds
        for (auto it = sounds_.begin(); it != sounds_.end(); ++it) {
            if (it->second) it->second->release();
        }
        sounds_.clear();
        channels_.clear();
        entityChannels_.clear();
        fadingChannels_.clear();

        if (system_) {
            system_->close();
            system_->release();
            system_ = nullptr;
            masterGroup_ = nullptr;
            musicGroup_ = nullptr;
            sfxGroup_ = nullptr;
        }
    }

    bool AudioSystem::loadSound(const std::string& id,
        const char* path,
        bool loop,
        bool stream,
        bool is3D) {
        if (!system_) return false;
        if (!path)     return false;

        // Check if file exists
        FILE* test = fopen(path, "rb");
        if (!test) {
            std::fprintf(stderr, "[Audio] File not found: %s\n", path);
            return false;
        }
        fclose(test);

        // Release old if reloading the same id
        unloadSound(id);

        FMOD_MODE mode = FMOD_DEFAULT;
        mode |= is3D ? FMOD_3D : FMOD_2D;
        if (is3D) mode |= FMOD_3D_LINEARROLLOFF;
        mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        mode |= stream ? FMOD_CREATESTREAM : 0;

        FMOD::Sound* snd = nullptr;
        FMOD_RESULT r = system_->createSound(path, mode, nullptr, &snd);
        check(r, "System::createSound");
        if (r != FMOD_OK || !snd) return false;

        sounds_[id] = snd;
        return true;
    }

    FMOD::Channel* AudioSystem::play(FMOD::Sound& sound, AudioType type, float volume, bool paused) {
        if (!system_) return nullptr;

        // Select the correct group
        FMOD::Channel* ch = nullptr;
        FMOD::ChannelGroup* targetGroup = (type == AudioType::MUSIC) ? musicGroup_ : sfxGroup_;

        // Pass the target group to playSound
        FMOD_RESULT r = system_->playSound(&sound, targetGroup, paused, &ch);
        check(r, "System::playSound");

        if (r != FMOD_OK || !ch) return nullptr;

        ch->setVolume(volume);
        return ch;
    }

    FMOD::Channel* AudioSystem::play3D(FMOD::Sound& sound, AudioType type, float volume,
        const Vector2D& worldPos, float minDistance, float maxDistance, bool paused) {
        if (!system_) return nullptr;

        FMOD::Channel* ch = nullptr;
        FMOD::ChannelGroup* targetGroup = (type == AudioType::MUSIC) ? musicGroup_ : sfxGroup_;

        // Start paused so we can set attrs first
        FMOD_RESULT r = system_->playSound(&sound, targetGroup, true, &ch);
        check(r, "System::playSound(3D)");
        if (r != FMOD_OK || !ch) return nullptr;

        FMOD_VECTOR pos{ worldPos.x, 0.0f, worldPos.y };
        FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };
        ch->set3DAttributes(&pos, &vel);

        // Force 3D + rolloff (sound should also be created as FMOD_3D for best results)
        ch->setMode(FMOD_3D | FMOD_3D_LINEARROLLOFF);
        ch->set3DMinMaxDistance(minDistance, maxDistance);

        ch->setVolume(volume);

        // Apply pause state
        ch->setPaused(paused ? true : false);
        if (!paused) ch->setPaused(false);

        return ch;
    }

    bool AudioSystem::playOneShot(const char* path, float volume) {
        if (!system_ || !path) return false;

        FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D | FMOD_LOOP_OFF;
        FMOD::Sound* snd = nullptr;

        FMOD_RESULT r = system_->createSound(path, mode, nullptr, &snd);
        check(r, "System::createSound(oneShot)");
        if (r != FMOD_OK || !snd) return false;

        FMOD::Channel* ch = nullptr;
        r = system_->playSound(snd, nullptr, false, &ch);
        check(r, "System::playSound(oneShot)");
        if (r == FMOD_OK && ch) {
            ch->setVolume(volume);
        }

        snd->release();
        return (r == FMOD_OK);
    }

    void AudioSystem::setMasterVolume(float volume) {
        if (masterGroup_) {
            masterGroup_->setVolume(volume);
        }
    }

    void AudioSystem::setMusicVolume(float volume) {
        if (musicGroup_) musicGroup_->setVolume(volume);
    }

    void AudioSystem::setSFXVolume(float volume) {
        if (sfxGroup_) sfxGroup_->setVolume(volume);
    }

    float AudioSystem::getMasterVolume() {
        float vol = 1.0f;
        if (masterGroup_) masterGroup_->getVolume(&vol);
        return vol;
    }

    float AudioSystem::getMusicVolume() {
        float vol = 1.0f;
        if (musicGroup_) musicGroup_->getVolume(&vol);
        return vol;
    }

    float AudioSystem::getSFXVolume() {
        float vol = 1.0f;
        if (sfxGroup_) sfxGroup_->getVolume(&vol);
        return vol;
    }

    void AudioSystem::stopAll(bool fadeOut) {
        if (fadeOut) {
            // Stop last-played channel per id
            for (auto it = channels_.begin(); it != channels_.end(); ++it) {
                if (it->second) {
                        FadeOutChannel(it->second, 1.0f);
                }
            }
        }
        else if (masterGroup_) {
            masterGroup_->stop();
            if (system_) system_->update(); // Force an update so the pause applies instantly
        }

        channels_.clear();
    }

    void AudioSystem::unloadSound(const std::string& id) {
        auto it = sounds_.find(id);
        if (it != sounds_.end()) {
            if (it->second) it->second->release();
            sounds_.erase(it);
        }
        // optionally forget last channel too
        auto ct = channels_.find(id);
        if (ct != channels_.end()) channels_.erase(ct);
    }

    void AudioSystem::OnButtonClicked(const ButtonClickedEvent& event) {
        // If button's action is not play sound, don't process
        if (event.action != ButtonAction::PLAYSOUND) {
            return;
        }

        // If no sound key provided, don't process the event
        if (event.stringParam.empty()) {
            return;
        }

        float volume = (event.floatParam > 0.0f) ? event.floatParam : 1.0f;

        Logger::Get().Info(DebugType::AudioSystem)
            << "Playing button sound: " << event.stringParam << std::endl;

        try {
            // Retrieve sound from ResourceManager using the string parameter from the button
            FMOD::Sound& sfx = GameManager::GetInstance().GetSystems().resourceManager_->get<FMOD::Sound>(event.stringParam);
            this->play(sfx, AudioType::SFX, volume, false);
        }
        catch (...) {
            Logger::Get().Error(DebugType::AudioSystem)
                << "Missing sound: " << event.stringParam << std::endl;
        }
    }

    void AudioSystem::SuspendAudio() {
        if (masterGroup_) {
            masterGroup_->setPaused(true);
            if (system_) system_->update(); // Force an update so the pause applies instantly
        }
    }

    void AudioSystem::ResumeAudio() {
        if (masterGroup_) {
            masterGroup_->setPaused(false);
            if (system_) system_->update(); // Force an update to resume instantly
        }
    }

    std::string AudioSystem::getRandomSound(std::string eventKey) {
        auto it = soundPaths_.find(eventKey);
        if (it != soundPaths_.end()) {
            const std::vector<std::string>& files = it->second;
            if (!files.empty()) {
                int index = 0;
                if (files.size() > 1) {
                    index = rand() % (int)files.size();
                }
                return files[index];
            }
        }
        return "";
    }

    void AudioSystem::FadeOutChannel(FMOD::Channel* channel, float duration) {
        if (!channel) return;

        float currentVol = 0.0f;
        channel->getVolume(&currentVol);

        // If duration is effectively zero, stop immediately
        if (duration <= 0.001f) {
            channel->stop();
            return;
        }

        // Calculate speed: Volume to lose per second
        float speed = currentVol / duration;

        // Add to map for processing in Update()
        fadingChannels_[channel] = speed;
    }

    void AudioSystem::PlayRandomSound(std::string eventKey, AudioType audiotype, float volume) {
        std::string soundPath = getRandomSound(eventKey);

        if (!soundPath.empty()) {
            // Load and Play
            try {
                FMOD::Sound& sound = GameManager::GetInstance().GetSystems().resourceManager_->get<FMOD::Sound>(soundPath);
                this->play(sound, audiotype, volume, false);
            }
            catch (...) {
                Logger::Get().Error(DebugType::AudioSystem) << "Missing sound: " << soundPath << std::endl;
            }
        }
    }

    void AudioSystem::PlayRandomSound3D(std::string eventKey, AudioType audiotype, float volume, Vector2D position, float minDist, float maxDist) {
        std::string soundPath = getRandomSound(eventKey);

        if (!soundPath.empty()) {
            // Load and Play
            try {
                FMOD::Sound& sound = GameManager::GetInstance().GetSystems().resourceManager_->get<FMOD::Sound>(soundPath);
                this->play3D(sound, audiotype, volume, position, minDist, maxDist, false);
            }
            catch (...) {
                Logger::Get().Error(DebugType::AudioSystem) << "Missing sound: " << soundPath << std::endl;
            }
        }
    }

    // ---------------- Reverb Controls ----------------

    void AudioSystem::EnableGroupReverb(AudioType type, bool enable) {
        FMOD::DSP* dsp = (type == AudioType::MUSIC) ? musicReverbDSP_ : sfxReverbDSP_;
        if (!dsp) return;
        dsp->setBypass(!enable);
    }

    void AudioSystem::SetGroupReverbWetDb(AudioType type, float wetDb) {
        FMOD::DSP* dsp = (type == AudioType::MUSIC) ? musicReverbDSP_ : sfxReverbDSP_;
        if (!dsp) return;
        dsp->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, wetDb);
    }

    void AudioSystem::SetGroupReverbDecay(AudioType type, float decayMs) {
        FMOD::DSP* dsp = (type == AudioType::MUSIC) ? musicReverbDSP_ : sfxReverbDSP_;
        if (!dsp) return;
        if (decayMs < 10.0f) decayMs = 10.0f;
        dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, decayMs);
    }

    // "Room size" approximation for SFXREVERB:
    // - bigger room => longer decay + higher density/diffusion
    void AudioSystem::SetGroupReverbRoom(AudioType type, float room01) {
        FMOD::DSP* dsp = (type == AudioType::MUSIC) ? musicReverbDSP_ : sfxReverbDSP_;
        if (!dsp) return;

        room01 = std::max(0.0f, std::min(1.0f, room01));

        float decayMs = 300.0f + room01 * 1700.0f; // 300..2000 ms
        float density = room01;                    // 0..1
        float diffusion = 0.2f + 0.8f * room01;    // 0.2..1

        dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, decayMs);
        dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, density);
        dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, diffusion);
    }

    // ---------------- Spatial Settings ----------------

    void AudioSystem::Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale) {
        if (!system_) return;
        distanceFactor_ = distanceFactor;
        system_->set3DSettings(dopplerScale, distanceFactor, rolloffScale);
    }

} // namespace Framework