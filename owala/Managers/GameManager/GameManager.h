/*********************************************************************//**
 * \file   GameManager.h
 *
 * \primary_author
 *      Jason Lou (80%)
 * \secondary_author
 *      Lur Pei Hao Stanley (20%)
 *
 * \brief   Manager responsible for managing game state, coordinating systems,
 *          and handling scene lifecycle.
 *
 * \details
 *      The GameManager is the central coordinator for the entire game engine.
 *      It manages:
 *        - Registration and access to all core systems
 *        - Scene lifecycle (loading, unloading, updates)
 *        - Play state management (editor vs runtime)
 *        - Native script execution
 *        - Scene transitions with fade effects
 *        - Button event handling for UI interactions
 *        - Global time dilation
 *        - Fade overlay system
 *
 *      The manager operates as a singleton and must be properly initialized
 *      with all required systems via RegisterSystemsToUse() before use.
 *
 *      Example Usage:
 *      \code
 *      // Get instance
 *      GameManager& gm = GameManager::GetInstance();
 *
 *      // Configure with game settings
 *      GameConfig config;
 *      config.startupScene = "MainMenu";
 *      config.loadingScene = "LoadingScreen";
 *      gm.Configure(config);
 *
 *      // Register all systems
 *      gm.RegisterSystemsToUse(rcm, ecs, gfx, win, input, physics, audio,
 *                              renderer, textRenderer, camera, ai, gui,
 *                              transformation, animSys);
 *
 *      // Initialize and run
 *      gm.Initialize();
 *      gm.LoadStartupScene();
 *
 *      // In game loop
 *      gm.Update();
 *
 *      // Shutdown
 *      gm.Shutdown();
 *      \endcode
 *
 * \note
 *      This is a singleton class. Access via GetInstance().
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the
 *      prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "GameConfig.h"
#include "SceneManager.h"
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "WindowsSystem.h"
#include "SpriteRenderSystem.h"
#include "TextRenderSystem.h"
#include "GuiSystem.h"
#include "Graphics.h"
#include "Core.h"
#include "AISystem.h"
#include "FileUtility.h"
#include "AnimationSystem.h"
#include "LayerManager.h"

#ifdef BUILD_EDITOR
#include "Editor.h"
#include <memory>
#endif

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>


namespace Framework {
    /**
     * @brief Enumeration of core system types for identification.
     */
    enum class SystemType : unsigned char {
        RESOURCE_MANAGER = 0,  //!< Resource management system
        ENTITY_STORAGE,        //!< ECS entity storage
        GRAPHICS,              //!< Graphics rendering system
        WINDOWS_SYSTEM,        //!< Window management
        INPUT_SYSTEM,          //!< Input handling
        PHYSICS_SYSTEM,        //!< Physics simulation
        AUDIO_SYSTEM           //!< Audio playback
    };

    /**
     * @brief Central coordinator for game state and system management.
     *
     * The GameManager is the heart of the engine, responsible for:
     * - Initializing and updating all registered systems
     * - Managing scene transitions and lifecycle
     * - Handling play/pause states in editor
     * - Executing native scripts
     * - Coordinating fade effects during scene changes
     * - Processing UI button events
     * - Maintaining global time dilation
     */
    class GameManager : public ISystem {
    public:
        // Aliases
        using SceneFunction = void(*)(void);  //!< Function pointer for scene callbacks

        // Re-export Framework::GameConfig as GameManager::GameConfig for backwards compatibility.
        using GameConfig = Framework::GameConfig;

    private:
        static GameManager* instance;         //!< Singleton instance pointer
        std::string nextSceneToLoad_ = "";    //!< Name of scene to load next
        std::function<void()> deferredTask_ = nullptr;  //!< Task to execute next frame
        std::function<void()> overlayDrawCallback_ = nullptr;  //!< Custom overlay drawing

        // Scene transition queue flags
        bool queuedLevelLoad_ = false;        //!< Level load pending
        bool queuedLevelReset_ = false;       //!< Level reset pending
        bool queuedResume_ = false;           //!< Resume from pause pending
        std::string queuedSceneUnload_;       //!< Scene to unload next frame
        std::string queuedLevelName_;          //!< Name of level to load

        // Additive scene management
        bool queuedAdditiveSwap_ = false;      //!< Additive scene swap pending
        std::string queuedAdditiveLoad_;        //!< Additive scene to load
        std::string queuedAdditiveUnload_;      //!< Additive scene to unload

        /**
         * @brief Container for scene lifecycle callbacks.
         */
        struct SceneCallBacks {
            SceneFunction fpInitialize = nullptr;  //!< Called when scene loads
            SceneFunction fpUpdate = nullptr;      //!< Called every frame while scene active
            SceneFunction fpFree = nullptr;        //!< Called when scene unloads
        };

        std::unordered_map<SceneGUID, SceneCallBacks> sceneCallbacks_;  //!< Scene callback registry
        std::unordered_set<SceneGUID> initializedScenes_;  //!< Scenes that have received Initialize()
        std::unordered_set<SceneGUID> autoConfiguredScenes_; //!< Scenes that received auto camera setup
        SceneGUID lastActiveScene_ = 0;  //!< Last active scene ID (for history tracking)

        /**
         * @brief Container for all registered system pointers.
         */
        struct RegisteredSystems {
            EntityStorage* ecs_ = nullptr;           //!< Entity Component System storage
            Graphics* gfx_ = nullptr;                 //!< Graphics rendering system
            WindowsSystem* win_ = nullptr;             //!< Window management system
            InputSystem* input_ = nullptr;             //!< Input handling system
            PhysicsSystem* physics_ = nullptr;         //!< Physics simulation system
            ResourceManager* resourceManager_ = nullptr; //!< Resource management
            AudioSystem* audio_ = nullptr;             //!< Audio playback system
            SpriteRenderSystem* spriteRenderer_ = nullptr; //!< Sprite rendering
            TextRenderSystem* textRenderer_ = nullptr;   //!< Text rendering
            AISystem* aiSystem_ = nullptr;              //!< AI system
            GuiSystem* guiSystem_ = nullptr;            //!< GUI system
            CameraSystem* camera_ = nullptr;             //!< Camera system
            TransformationSystem* transformation_ = nullptr; //!< Transform calculations
            AnimationSystem* animationSystem_ = nullptr;    //!< Animation system
        };

        RegisteredSystems systems_;  //!< All registered system pointers

#ifdef BUILD_EDITOR
        Editor::Editor* editor_ = nullptr; //!< ImGui Editor instance

        /* Play State Management */
        bool hasECSSnapshot = false;           //!< Whether we have a snapshot of editor ECS
        EntityStorage* editorECS_ = nullptr;    //!< Pointer to original editor ECS
        std::unique_ptr<EntityStorage> runtimeECS_ = nullptr; //!< Runtime ECS copy
        Entity nextEntityIDSnapshot = 1u;       //!< Saved next entity ID
#endif

        float deltaTime = 0.0f;                 //!< Time since last frame
        float globalTimeDilation = 1.0f;         //!< Global time scale (1.0 = normal)

        /**
         * @brief Private constructor for singleton pattern.
         */
        GameManager() = default;
        ~GameManager() = default;

        // Internal Checkers
        void CheckSystems() const;  //!< Verify all required systems are registered

        GameConfig config_;  //!< Game configuration loaded from file

        CoreEngine* engine_ = nullptr; //!< Pointer to core engine for system time access

        // System profiler display members
        float profilerUpdateTimer_ = 0.0f;           //!< Timer for profiler updates
        std::vector<SystemTime> profilerDisplayData_; //!< Cached profiler data

        // Internal Helpers
        void UpdateNativeScripts();  //!< Update all native script components

        void PreloadSceneAssets(const std::string& sceneName);  //!< Preload textures for a scene

        SceneGUID previousScene = 0;  //!< Previously active scene

        // Black Fade Overlay system
        bool  fadeActive_ = false;     //!< Whether fade is in progress
        bool  fadeOut_ = false;        //!< true = clear->black, false = black->clear
        float fadeAlpha_ = 0.0f;        //!< Current fade alpha (0-1)
        float fadeDuration_ = 0.5f;     //!< Duration of fade in seconds
        float fadeTimer_ = 0.0f;        //!< Current fade timer
        std::function<void()> fadeOnComplete_ = nullptr;  //!< Callback when fade completes

        void TickFadeOverlay(float dt);  //!< Update fade animation

    public:
        // GLOBAL STATES
        int testState = 0;  //!< Test state variable (debug use)

        // Singletons should not be cloneable or assignable.
        GameManager(const GameManager&) = delete;
        GameManager& operator=(const GameManager&) = delete;

        /**
         * @brief Gets the singleton instance of GameManager.
         *
         * @return GameManager& Reference to the singleton instance
         */
        static GameManager& GetInstance();

        /**
         * @brief Checks if a GameManager instance exists.
         *
         * @return true if instance exists, false otherwise
         */
        static bool Exists();

        /**
         * @brief Destroys the current GameManager instance.
         */
        static void DestroyInstance();

        /* ======================================================================
         * Getters & Setters
         * ====================================================================== */

         /**
          * @brief Gets delta time (time since last frame).
          *
          * @return float Delta time in seconds
          */
        float GetDeltaTime() const { return deltaTime; }

        /**
         * @brief Gets all registered systems.
         *
         * @return RegisteredSystems& Reference to systems container
         */
        RegisteredSystems& GetSystems() { return systems_; }

        /**
         * @brief Gets all registered systems (const version).
         *
         * @return const RegisteredSystems& Const reference to systems container
         */
        const RegisteredSystems& GetSystems() const { return systems_; }

        /**
         * @brief Gets the EntityStorage instance.
         *
         * @return EntityStorage& Reference to ECS storage
         */
        EntityStorage& GetECS() { return *systems_.ecs_; };

        /**
         * @brief Gets the EntityStorage instance (const version).
         *
         * @return const EntityStorage& Const reference to ECS storage
         */
        const EntityStorage& GetECS() const { return *systems_.ecs_; }

        /**
         * @brief Sets the overlay draw callback.
         *
         * @param cb Function to call for custom overlay rendering
         */
        void SetOverlayDrawCallback(std::function<void()> cb) { overlayDrawCallback_ = cb; }

        /**
         * @brief Gets current fade alpha value.
         *
         * @return float Current alpha (0-1)
         */
        float GetFadeAlpha() const { return fadeAlpha_; }

        /**
         * @brief Sets the core engine pointer.
         *
         * @param engine Reference to core engine
         */
        void SetEngine(CoreEngine& engine) { engine_ = &engine; }

        /**
         * @brief Configures the game with settings from file.
         *
         * @param config GameConfig structure with startup settings
         */
        void Configure(const GameConfig& config);

        /**
         * @brief Gets the current game configuration (mutable).
         *
         * @return GameConfig& Reference to config
         */
        GameConfig& GetConfig() { return config_; }

        /**
         * @brief Gets the current game configuration (const version).
         *
         * @return const GameConfig& Const reference to config
         */
        const GameConfig& GetConfig() const { return config_; }

        /* ======================================================================
         * Fade System
         * ====================================================================== */

         /**
          * @brief Starts a fade-out (screen to black).
          *
          * @param duration Duration in seconds
          * @param onComplete Callback when fade completes
          */
        void FadeOut(float duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Starts a fade-in (black to screen).
         *
         * @param duration Duration in seconds
         * @param onComplete Callback when fade completes
         */
        void FadeIn(float duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Checks if a fade is in progress.
         *
         * @return true if fading, false otherwise
         */
        bool IsFading() const { return fadeActive_; }

        /* ======================================================================
         * System Registration
         * ====================================================================== */

         /**
          * @brief Registers all core systems with the GameManager.
          *
          * Must be called before Initialize().
          *
          * @param rcm Resource manager
          * @param ecs Entity storage
          * @param gfx Graphics system
          * @param win Window system
          * @param input Input system
          * @param physics Physics system
          * @param audio Audio system
          * @param renderer Sprite render system
          * @param textRenderer Text render system
          * @param camera Camera system
          * @param ai AI system
          * @param gui GUI system
          * @param transformation Transformation system
          * @param animSys Animation system
          */
        void RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx,
            WindowsSystem& win, InputSystem& input, PhysicsSystem& physics,
            AudioSystem& audio, SpriteRenderSystem& renderer, TextRenderSystem& textRenderer,
            CameraSystem& camera, AISystem& ai, GuiSystem& gui, TransformationSystem& transformation,
            AnimationSystem& animSys);

#ifdef BUILD_EDITOR
        /**
         * @brief Registers the editor instance with GameManager.
         *
         * @param editor Reference to editor instance
         */
        void RegisterEditor(Editor::Editor& editor);

        /**
         * @brief Gets the editor instance.
         *
         * @return Editor::Editor* Pointer to editor, or nullptr if not in editor build
         */
        Editor::Editor* GetEditor() const { return editor_; }

        /* Play State Management */
        void OnBeginPlay();   //!< Called when entering play mode in editor
        void OnEndPlay();     //!< Called when exiting play mode in editor
#endif

        /* ======================================================================
         * Scene Management APIs
         * ====================================================================== */

         /**
          * @brief Registers lifecycle callbacks for a scene.
          *
          * @param sceneIndex Scene GUID
          * @param initFunction Initialize callback
          * @param updateFunction Update callback
          * @param freeFunction Free/unload callback
          */
        void RegisterScriptToScene(SceneGUID sceneIndex, SceneFunction initFunction, SceneFunction updateFunction, SceneFunction freeFunction);

        /**
         * @brief Unregisters lifecycle callbacks for a scene.
         *
         * @param sceneIndex Scene GUID
         */
        void UnregisterScriptFromScene(SceneGUID sceneIndex);

        /**
         * @brief Checks if game is in play mode.
         *
         * @return true if playing, false if in editor mode
         */
        bool IsGamePlaying() const;

        /**
         * @brief Sets the overlay callback for custom drawing.
         *
         * @param cb Function to call for overlay rendering
         */
        void SetOverlayCallback(std::function<void()> cb) { overlayDrawCallback_ = cb; }

        /**
         * @brief Sets the next scene to load.
         *
         * @param sceneName Name of scene
         */
        void SetNextScene(const std::string& sceneName) { nextSceneToLoad_ = sceneName; }

        /**
         * @brief Gets the name of the next scene to load.
         *
         * @return std::string Scene name
         */
        std::string GetNextScene() const { return nextSceneToLoad_; }

        /**
         * @brief Initiates a level load with fade transition.
         *
         * @param levelName Name of level to load
         */
        void InitiateLevelLoad(const std::string& levelName);

        /**
         * @brief Checks if a scene is a playable level (not a menu).
         *
         * @param name Scene name
         * @return true if level scene, false if menu scene
         */
        bool IsLevelScene(const std::string& name);

        /**
         * @brief Queues a level load to be processed next frame.
         *
         * Use this when calling from inside a scene update callback to avoid
         * modifying the loaded-scenes list while it's being iterated.
         *
         * @param levelName Name of level to load
         */
        void QueueLevelLoad(const std::string& levelName) {
            queuedLevelName_ = levelName;
            queuedLevelLoad_ = true;
        }

        /**
         * @brief Gets the previously active scene.
         *
         * @return SceneGUID GUID of previous scene
         */
        SceneGUID GetPrevScene() const { return previousScene; }

        /**
         * @brief Loads the startup scene defined in GameConfig.
         */
        void LoadStartupScene();

        /* ======================================================================
         * Game Cycle Functions (ISystem implementation)
         * ====================================================================== */

        void Initialize() override;
        void Update() override;
        void Shutdown() override;
    };
}