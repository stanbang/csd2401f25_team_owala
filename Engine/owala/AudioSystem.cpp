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

		// Load in audio file names
        FileIOSystem::LoadAudioFileNames(soundPaths_);

        // Subscribe to Player Events
        MessageBus::Instance().Subscribe<PlayerJumpedEvent>([this](const PlayerJumpedEvent& e) {this->PlayRandomSound("Player_Jump", AudioType::SFX, e.volume); });
        MessageBus::Instance().Subscribe<PlayerLandEvent>([this](const PlayerLandEvent&) {this->PlayRandomSound("Player_Land", AudioType::SFX, 1.0f);  });
        MessageBus::Instance().Subscribe<PlayerWalkEvent>([this](const PlayerWalkEvent& e) {this->PlayLoopSound(e.player, "Player_Walk", AudioType::SFX, 1.0f, e.isWalking);  });

        MessageBus::Instance().Subscribe<PlayerDamagedEvent>([this](const PlayerDamagedEvent&) { this->PlayRandomSound("Player_Hurt", AudioType::SFX, 1.0f); });
        MessageBus::Instance().Subscribe<PlayerDeathEvent>([this](const PlayerDeathEvent& e) {
            if (!e.deathAnimationFinished) {  // Only play sound on the initial death, not after animation ends
                this->PlayRandomSound("Player_Death", AudioType::SFX, 1.0f);
            }
        });
       
        // Subscribe to Bird Enemy Events
        MessageBus::Instance().Subscribe<BirdWakeEvent>([this](const BirdWakeEvent&) {this->PlayRandomSound("Bird_Wake", AudioType::SFX, 0.3f); });
        MessageBus::Instance().Subscribe<BirdChaseEvent>([this](const BirdChaseEvent& e) {
            this->PlayLoopSound(e.entityID, "Bird_Chase", AudioType::SFX, 0.3f, e.isCurrentlyChasing); 
        });
        MessageBus::Instance().Subscribe<BirdAttackEvent>([this](const BirdAttackEvent&) {this->PlayRandomSound("Bird_Attack", AudioType::SFX, 0.3f); });

        // Subscribe to World Rotation Event
        MessageBus::Instance().Subscribe<WorldRotationEvent>([this](const WorldRotationEvent&) {this->PlayRandomSound("Stage_Rotate", AudioType::SFX, 1.0f); });
        // Subscribe to Trap Platform Land Event
        MessageBus::Instance().Subscribe<TrapPlatformLandEvent>([this](const TrapPlatformLandEvent& e) {this->TriggerProximitySFX(e.position, "Platform_Land", AudioType::SFX); });
        // Subscribe to Curtains Open/Close Event
        MessageBus::Instance().Subscribe<CurtainsOpenEvent>([this](const CurtainsOpenEvent&) {this->PlayRandomSound("Curtains_Open", AudioType::SFX, 1.0f); });
        MessageBus::Instance().Subscribe<CurtainsCloseEvent>([this](const CurtainsCloseEvent&) {this->PlayRandomSound("Curtains_Close", AudioType::SFX, 1.0f);});
        
		// Subscribe to ButtonClickedEvent
        MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& e) {this->OnButtonClicked(e); });
    }

    void AudioSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        ResourceManager& resourceManager_ = *gm.GetSystems().resourceManager_;
		EntityManager entityMgr(gm.GetECS());
        if (system_) {
            FMOD_RESULT r = system_->update();
            check(r, "System::update");
        }

        for (auto it = entityChannels_.begin(); it != entityChannels_.end(); ) {
            Entity ownerEntity = it->first;
            FMOD::Channel* channel = it->second;

            // Check if the entity still "Exists" in the scene logic
            bool isEntityActive = entityMgr.HasComponent(ownerEntity, AudioComponentType) &&
                entityMgr.HasComponent(ownerEntity, LoadedTagType) && entityMgr.IsActive(ownerEntity);

            // Check if the channel is actually still playing
            bool isChannelPlaying = false;
            if (channel) {
                channel->isPlaying(&isChannelPlaying);
            }

            // REMOVE IF: Entity is dead OR Sound finished naturally
            if (!isEntityActive || !isChannelPlaying) {
                if (channel) {
                    channel->stop(); // Cut the audio immediately
                }
                it = entityChannels_.erase(it); // Remove from map
            }
            else {
                ++it;
            }
        }

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, AudioComponentType)) {

			// Check if Entity is active in the scene. Loads if active.
			if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
            // Check if entity is tied to a button
			if (entityMgr.HasComponent(entity, ButtonType)) {
                continue; //Skip button entities, handle separately.
			}

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
                    /*FMOD::Channel* ch = nullptr;
                    system_->playSound(&sound, nullptr, false, &ch);
                    if (ch) {
                        ch->setVolume(comp.volume);
                        channels_[comp.soundKey] = ch;
                    }*/

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
        // Stop and release channels is not strictly necessary: releasing sounds
        // and closing the system will stop them, but we�ll do a clean pass
        stopAll();

        // Release all cached sounds
        for (std::unordered_map<std::string, FMOD::Sound*>::iterator it = sounds_.begin();
            it != sounds_.end(); ++it) {
            if (it->second) it->second->release();
        }
        sounds_.clear();
        channels_.clear();

        if (system_) {
            system_->close();
            system_->release();
            system_ = nullptr;
            masterGroup_ = nullptr;
        }
    }

	//void AudioSystem::SetResourceManager(ResourceManager& rcm) {
	//	resourceManager_ = &rcm;
	//}

	// Access the underlying FMOD system (for advanced use)
    FMOD::System* AudioSystem::getFMODSystem() const { return system_; }

    bool AudioSystem::loadSound(const std::string& id,
        const char* path,
        bool loop,
        bool stream) {
        if (!system_) return false;
        if (!path)     return false;

        if (!system_) {
            std::fprintf(stderr, "[Audio] System not initialized\n");
            return false;
        }
        if (!path) {
            std::fprintf(stderr, "[Audio] Null path for id '%s'\n", id.c_str());
            return false;
        }

        // Check if file exists
        FILE* test = fopen(path, "rb");
        if (!test) {
            std::fprintf(stderr, "[Audio] File not found: %s\n", path);
            return false;
        }
        fclose(test);

        // Release old if reloading the same id
        unloadSound(id);

        FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D;
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
        if (!system_) return nullptr; // Return nullptr on failure

        // Select the correct group
        FMOD::Channel* ch = nullptr;
        FMOD::ChannelGroup* targetGroup = (type == AudioType::MUSIC) ? musicGroup_ : sfxGroup_;
        // Pass the target group to playSound
        FMOD_RESULT r = system_->playSound(&sound, targetGroup, paused, &ch);
        check(r, "System::playSound");

        if (r != FMOD_OK || !ch) return nullptr;

        // ? actually apply the volume now
        ch->setVolume(volume);

        // Optionally remember the channel (if you need to stop or modify later)
        // channels_[<some-id>] = ch;

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

        // release when finished; FMOD will free after playback if no channels reference it
        snd->release();
        return (r == FMOD_OK);
    }

    void AudioSystem::setMasterVolume(float volume) {
        if (masterGroup_) {
            masterGroup_->setVolume(volume);
        }
    }

    void AudioSystem::stopAll() {
   
        // Stop last-played channel per id
        for (std::unordered_map<std::string, FMOD::Channel*>::iterator it = channels_.begin();
            it != channels_.end(); ++it) {
            if (it->second) {
                it->second->stop();
            }
        }
        channels_.clear();
    }

    void AudioSystem::unloadSound(const std::string& id) {
        std::unordered_map<std::string, FMOD::Sound*>::iterator it = sounds_.find(id);
        if (it != sounds_.end()) {
            if (it->second) it->second->release();
            sounds_.erase(it);
        }
        // optionally forget last channel too
        std::unordered_map<std::string, FMOD::Channel*>::iterator ct = channels_.find(id);
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

    float AudioSystem::GetProximityVol(const Vector2D& pos, float maxVol) {
        // Get Player Position
        Vector2D playerPos;
        EntityManager entityMgr = EntityManager(GameManager::GetInstance().GetECS());
        std::vector<Entity> playerEntities = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);

        if (!playerEntities.empty()) {
            Entity playerID = playerEntities[0];
            playerPos = entityMgr.GetComponent<Transform>(playerID).position;
        }
        else {
            return 0.0f;
        }

        // Calculate Distance
        float distance = (pos - playerPos).Magnitude();

        // Calculate Volume (Linear Falloff)
        float maxHearingRange = 700.0f; // TODO: move this hardcoded value to player.json
        float volume = 0.0f;

        if (distance < maxHearingRange) {
            volume = maxVol - (distance / maxHearingRange);
        }

        // Play Sound
        if (volume > 0.05f) {
            return volume;
        }

        return 0.0f;
    }

    std::string AudioSystem::getRandomSound(std::string eventKey) {
        if (soundPaths_.find(eventKey) != soundPaths_.end()) {
            const std::vector<std::string>& files = soundPaths_[eventKey];

            if (!files.empty()) {
                int index = 0;

                // Pick a random index if there's more than 1 sound file in the array
                if (files.size() > 1) {
                    index = rand() % files.size();
                }

                return files[index];
            }
        }
        return "";
    }

    void AudioSystem::TriggerProximitySFX(const Vector2D& pos, const std::string eventKey, AudioType audiotype, float maxVol) {
        float volume = GetProximityVol(pos, maxVol);

        if (volume > 0.0f) {
            this->PlayRandomSound(eventKey, audiotype, volume);
        }
    }

    void AudioSystem::PlayRandomSound(std::string eventKey, AudioType audiotype, float volume) {
        std::string soundPath = getRandomSound(eventKey);

        if (!soundPath.empty()) {
            // Load and Play
            try {
                FMOD::Sound& sound = GameManager::GetInstance().GetSystems().resourceManager_->get<FMOD::Sound>(soundPath);
                float vol = volume;
                this->play(sound, audiotype, volume, false);
            }
            catch (...) { 
                Logger::Get().Error(DebugType::AudioSystem) << "Missing sound: " << soundPath << std::endl;
            }
        }
    }

    void AudioSystem::PlayLoopSound(Entity entity, std::string eventKey, AudioType audiotype, float volume, bool play, bool proximity, Vector2D pos) {
        std::string soundPath = getRandomSound(eventKey);
        if (soundPath.empty()) return;

        std::string uniqueMapKey = std::to_string(entity) + "_" + soundPath;

        try {
            // Check if we already have a record of this sound
            auto it = channels_.find(uniqueMapKey);
            FMOD::Channel* existingChannel = (it != channels_.end()) ? it->second : nullptr;

            // --- STOP REQUEST ---
            if (!play) {
                if (existingChannel) {
                    bool isPlaying = false;
                    existingChannel->isPlaying(&isPlaying);

                    if (isPlaying) {
                        existingChannel->stop();
                    }

                    // Remove this specific channel from the map
                    channels_.erase(it);
                }
                return;
            }

            // --- PLAY REQUEST ---
            bool isAlreadyPlaying = false;
            if (existingChannel) {
                existingChannel->isPlaying(&isAlreadyPlaying);
            }

            float vol = volume;
            if (proximity) {
                vol = GetProximityVol(pos, volume);
            }

            if (isAlreadyPlaying) {
                existingChannel->setVolume(vol);
            }
            else {
                FMOD::Sound& sound = GameManager::GetInstance().GetSystems().resourceManager_->getLoopAudio(soundPath);
                FMOD::Channel* newChannel = this->play(sound, audiotype, vol, false);

                if (newChannel) {
                    // Force it to loop just in case the wrapper didn't
                    newChannel->setMode(FMOD_LOOP_NORMAL);
                    newChannel->setLoopCount(-1);

                    channels_[uniqueMapKey] = newChannel;
                }
            }
        }
        catch (...) {
            Logger::Get().Error(DebugType::AudioSystem) << "Missing sound: " << soundPath << std::endl;
        }
    }

} // namespace Framework