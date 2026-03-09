#include "Core/Application.h"
#include "Core/Core.h" 
#include "Managers/GameManager/GameManager.h"
#include "FileIO/FileUtility.h"

// Game-specific systems in Game/Sources folder)
#include "PlayerSystem.h"

// Scene script headers (only scenes that have Initialize/Update/Free)
#include "LevelSelection.h"
#include "SplashScreen.h"
#include "CutsceneIntro.h"
#include "HowToPlay.h"
#include "TransitionController.h"
#include "TestScene.h"
#include "MessageBus.h"
#include "GameMessages.h"

namespace Framework {

    class StageBoundApp : public Application {
    private:
        CoreEngine& m_Engine;
        PlayerSystem* m_PlayerSystem;

    public:
        StageBoundApp(CoreEngine& engine) : m_Engine(engine) {}

        void Initialize() override {
            std::cout << "--- StageBound Application Initializing! ---\n";

            // Instantiate Game-Specific Systems
            m_PlayerSystem = new PlayerSystem();

            // Inject Game Systems into the Engine
            m_Engine.AddSystem(m_PlayerSystem);

			// Initialize Game Systems
            m_PlayerSystem->Initialize();

            // Restore transition state that was previously in GameManager::Initialize()
            TransitionController::Reset();

            // Register Scene Lifecycle Callbacks
            GameManager& gm = GameManager::GetInstance();
            SceneManager& sceneMgr = SceneManager::GetInstance();

            // Game tells the engine what its scene names are, engine knows nothing
            Framework::GameManager::GameConfig config = Framework::FileIOSystem::LoadGameConfig();
            gm.Configure(config);

            // Register scripts to scene
            auto RegisterScene = [&](const std::string& name,
                GameManager::SceneFunction init,
                GameManager::SceneFunction update,
                GameManager::SceneFunction free) {
                    const SceneAsset& scene = sceneMgr.GetSceneByName(name, true);
                    if (scene.IsValid()) {
                        gm.RegisterScriptToScene(scene.sceneIndex, init, update, free);
                    }
                };

            // Menu / UI scenes
            RegisterScene("LevelSelection", LevelSelection::Initialize, LevelSelection::Update, LevelSelection::Free);
            RegisterScene("SplashScreen", SplashScreen::Initialize, SplashScreen::Update, SplashScreen::Free);
            RegisterScene("CutsceneIntro", CutsceneIntro::Initialize, CutsceneIntro::Update, CutsceneIntro::Free);
            RegisterScene("HowToPlay", HowToPlay::Initialize, HowToPlay::Update, HowToPlay::Free);
        
            // Gameplay levels - TestScene handles camera setup, player spawn, etc.
            RegisterScene("Tutorial 1", TestScene::Initialize, TestScene::Update, TestScene::Free);
            RegisterScene("Tutorial 2", TestScene::Initialize, TestScene::Update, TestScene::Free);
            RegisterScene("Level 1", TestScene::Initialize, TestScene::Update, TestScene::Free);
            RegisterScene("Level 2", TestScene::Initialize, TestScene::Update, TestScene::Free);

            // Now that config and callbacks are set, load the startup scene.
            // In editor mode the editor manages scene loading — skip this.
            if (!m_Engine.IsEditorMode()) {
                gm.LoadStartupScene();
            }
        }

        void Update() override {
            // Game-level high-level logic can go here (runs every frame)
        }

        void Shutdown() override {
            // The engine's DestroySystems() automatically deletes m_PlayerSystem 
            // because we added them via AddSystem().
            std::cout << "--- StageBound Application Shutting Down! ---\n";
        }
    };

    // Define the external function the Engine's main.cpp is looking for!
    Application* CreateApplication(CoreEngine& engine) {
        return new StageBoundApp(engine);
    }

} // namespace Framework