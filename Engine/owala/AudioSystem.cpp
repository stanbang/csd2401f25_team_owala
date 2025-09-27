#include "AudioSystem.h"
#include "EntityManager.h"
#include <cstdio>   // fprintf

namespace Framework {

    AudioSystem::AudioSystem(EntityStorage& storage) : entityStorage_(&storage) {}

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
    }

    void AudioSystem::Update(float /*dt*/) {
		EntityManager entityMgr(*entityStorage_);
        if (system_) {
            FMOD_RESULT r = system_->update();
            check(r, "System::update");
        }

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(AudioComponentType)) {
			AudioComponent& comp = entityMgr.GetComponent<AudioComponent>(entity);
            if (comp.soundKey.empty()) continue;

            try {
                FMOD::Sound& sound = resourceManager_->get<FMOD::Sound>(comp.soundKey);

                // check if sound is already playing
                bool isPlaying = false;
                auto it = channels_.find(comp.soundKey);
                if (it != channels_.end() && it->second) {
                    it->second->isPlaying(&isPlaying);
                }

                // play if not playing
                if (!isPlaying) {
                    FMOD::Channel* ch = nullptr;
                    system_->playSound(&sound, nullptr, false, &ch);
                    if (ch) {
                        ch->setVolume(comp.volume);
                        channels_[comp.soundKey] = ch;
                    }
                }

            }
            catch (const GameException& e) {
                std::fprintf(stderr, "[Audio] Sound '%s' not found in ResourceManager\n", comp.soundKey.c_str());
            }
        }
    }


    void AudioSystem::Shutdown() {
        // Stop and release channels is not strictly necessary: releasing sounds
        // and closing the system will stop them, but we’ll do a clean pass
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

	void AudioSystem::SetResourceManager(ResourceManager& rcm) {
		resourceManager_ = &rcm;
	}

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

    bool AudioSystem::play(FMOD::Sound& sound, float volume, bool paused) {
        if (!system_) return false;

        /*std::unordered_map<std::string, FMOD::Sound*>::iterator it = sounds_.find(id);
        if (it == sounds_.end() || !it->second) {
            std::fprintf(stderr, "[Audio] sound id '%s' not loaded.\n", id.c_str());
            return false;
        }*/

        FMOD::Channel* ch = nullptr;
        FMOD_RESULT r = system_->playSound(&sound, nullptr, paused, &ch);
        check(r, "System::playSound");
        /*if (r != FMOD_OK || !ch) return false;

        ch->setVolume(volume);
        channels_[id] = ch; // remember last channel for that id (optional)*/
        return true;
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

} // namespace Framework