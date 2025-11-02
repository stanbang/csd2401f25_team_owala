/*****************************************************************//**
 * \file   AudioSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 * \brief
 *   Audio system wrapper around FMOD for loading, playing, and managing
 *   sounds within the Framework ECS. Provides simple caching by ID,
 *   fire-and-forget playback, master volume control, and bulk stop.
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

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <unordered_map>

namespace Framework {
    class AudioSystem : public ISystem {
    public:
        /**
         * \brief Construct the audio system bound to the ECS storage.
         * \param storage Reference to the entity storage (kept as a raw pointer internally).
         */
        AudioSystem(EntityStorage& storage);

        /// \brief Default virtual destructor.
        ~AudioSystem() override = default;

        // ---------------- ISystem ----------------

        /**
         * \brief Initialize FMOD system and master channel group.
         * \details Creates FMOD::System, sets up the master group, and prepares
         *          internal caches. Safe to call once before any audio use.
         */
        void Initialize() override;

        /**
         * \brief Pump the audio mixer and perform per-frame maintenance.
         * \param dt Delta time in seconds since last frame (may be unused by FMOD but
         *           kept for consistency with ISystem).
         * \details Internally calls FMOD::System::update(). Must be called every frame.
         */
        void Update() override;

        /**
         * \brief Shut down FMOD and release all cached sounds/channels.
         * \details Unloads sounds, releases channel group and FMOD system.
         *          After Shutdown(), the instance must be re-Initialized() to use again.
         */
        void Shutdown() override;

        /**
         * \brief Provide a ResourceManager for resolving paths/handles if needed.
         * \param rcm Resource manager reference stored internally (no ownership).
         */
        void SetResourceManager(ResourceManager& rcm);

        // -------------- Simple API --------------

        /**
         * \brief Access the underlying FMOD system pointer.
         * \return FMOD::System* raw pointer (may be null before Initialize()).
         */
        FMOD::System* getFMODSystem() const;

        /**
         * \brief Load (and cache) a sound by ID.
         * \param id     Unique key used to cache/retrieve this sound.
         * \param path   File system path to the audio asset.
         * \param loop   If true, the sound will loop when played.
         * \param stream If true, use FMOD streaming (recommended for large/bgm files).
         * \return True on success, false on failure (check logs for FMOD errors).
         *
         * \post On success, the sound is available in \c sounds_ under \c id.
         */
        bool loadSound(const std::string& id,
            const char* path,
            bool loop = false,
            bool stream = false);

        /**
         * \brief Play a previously created FMOD::Sound.
         * \param sound   Reference to a valid FMOD::Sound to play.
         * \param volume  Linear volume in [0,1].
         * \param paused  If true, start paused (caller can unpause later).
         * \return True if playback was successfully started.
         *
         * \note This overload does not look up by ID; it uses the provided sound directly.
         * \see playOneShot(), loadSound()
         */
        bool play(FMOD::Sound& sound, float volume = 1.0f, bool paused = false);

        /**
         * \brief Fire-and-forget playback: load a sound from \p path and play once.
         * \param path    File system path to the audio asset.
         * \param volume  Linear volume in [0,1].
         * \return True if the sound was loaded and played successfully.
         *
         * \details The sound is not cached; it will be released after playback ends.
         *          Use this for short SFX you don't need to reuse.
         */
        bool playOneShot(const char* path, float volume = 1.0f);

        /**
         * \brief Set master channel group volume.
         * \param volume Linear volume in [0,1]. Values outside range will be clamped by FMOD.
         * \note Affects all channels routed through \c masterGroup_.
         */
        void setMasterVolume(float volume);

        /**
         * \brief Stop all currently playing channels started by this system.
         * \details Iterates known channels and stops them; safe to call anytime.
         */
        void stopAll();

        /**
         * \brief Unload and remove a cached sound by ID.
         * \param id Key of the sound to unload.
         * \details If the sound is currently playing, its channel will keep a handle
         *          to the FMOD::Sound until FMOD finishes/cleans up; future plays by
         *          this ID will fail until reloaded.
         */
        void unloadSound(const std::string& id);

        /**
         * \brief Helper for FMOD error handling.
         * \param r     FMOD result code.
         * \param where Human-readable context string (e.g., function name).
         * \throws std::runtime_error if \p r indicates failure.
         */
        static void check(FMOD_RESULT r, const char* where);

    private:
        /// \name FMOD Core Pointers
        ///@{
        FMOD::System* system_ = nullptr;  //!< Owned FMOD system (created in Initialize()).
        FMOD::ChannelGroup* masterGroup_ = nullptr;  //!< Master channel group used for global control.
        ///@}

        /// \name Engine Integration
        ///@{
        ResourceManager* resourceManager_ = nullptr; //!< Non-owning; may be null if not set.
        EntityStorage* entityStorage_ = nullptr; //!< Non-owning pointer to ECS storage.
        ///@}

        /// \name Caches
        ///@{
        std::unordered_map<std::string, FMOD::Sound*>   sounds_;   //!< Cached sounds by ID (owned by FMOD; released on unload/Shutdown()).
        std::unordered_map<std::string, FMOD::Channel*> channels_; //!< Last started channel per ID (optional bookkeeping).
        ///@}
    };

} // namespace Framework

