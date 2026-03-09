#include "Core/Application.h"
#include "Core/Core.h"
#include "Managers/GameManager/GameManager.h"
#include "FileIO/FileUtility.h"

namespace Framework {

    class newproj1App : public Application {
    public:
        newproj1App(CoreEngine& engine) : m_Engine(engine) {}

        void Initialize() override {
            auto& gm = GameManager::GetInstance();

            // Load scene categorization from Data/GameConfig.json.
            // Edit it via Edit > Project Settings in the editor.
            GameManager::GameConfig config = FileIOSystem::LoadGameConfig();
            gm.Configure(config);

            // TODO: Register per-scene script callbacks, e.g.:
            // gm.RegisterScriptToScene(sceneIndex, Init, Update, Free);

            // Load the startup scene (skipped automatically in editor mode).
            if (!m_Engine.IsEditorMode()) {
                gm.LoadStartupScene();
            }
        }

        void Update() override {}

        void Shutdown() override {}

    private:
        CoreEngine& m_Engine;
    };

    Application* CreateApplication(CoreEngine& engine) {
        return new newproj1App(engine);
    }

} // namespace Framework
