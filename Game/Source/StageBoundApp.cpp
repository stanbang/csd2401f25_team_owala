#include "Core/Application.h"
#include "Core/Core.h" 
#include "Managers/GameManager/GameManager.h"

// Include your game-specific systems! 
// (These should now be located inside your Game/ folder)
#include "PlayerSystem.h"
#include "LevelResetSystem.h"

namespace Framework {

    class StageBoundApp : public Application {
    private:
        CoreEngine& m_Engine;
        PlayerSystem* m_PlayerSystem;
        LevelResetSystem* m_LevelResetSystem;

    public:
        StageBoundApp(CoreEngine& engine) : m_Engine(engine) {}

        void Initialize() override {
            std::cout << "--- StageBound Application Initializing! ---\n";

            // 1. Instantiate Game-Specific Systems
            m_PlayerSystem = new PlayerSystem();
            m_LevelResetSystem = new LevelResetSystem();

            // 2. Inject Game Systems into the Engine
            m_Engine.AddSystem(m_PlayerSystem);
            m_Engine.AddSystem(m_LevelResetSystem);

            // Load specific scenes here, eg this is how to load a starting scene
            // SceneManager::GetInstance().LoadScene("MainMenu");
        }

        void Update() override {
            // Game-level high-level logic can go here (runs every frame)
        }

        void Shutdown() override {
            // The engine's DestroySystems() automatically deletes m_PlayerSystem 
            // and m_LevelResetSystem because we added them via AddSystem().
            std::cout << "--- StageBound Application Shutting Down! ---\n";
        }
    };

    // Define the external function the Engine's main.cpp is looking for!
    Application* CreateApplication(CoreEngine& engine) {
        return new StageBoundApp(engine);
    }

} // namespace Framework