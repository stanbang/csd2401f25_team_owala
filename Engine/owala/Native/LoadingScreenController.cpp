#include "LoadingScreenController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Components.h"
#include "EntityManager.h" // <--- ADD THIS to use EntityManager
#include <string>
#include <iomanip>
#include <sstream>

namespace Framework {

    REGISTER_SCRIPT(LoadingScreenController, "LoadingScreenController")

        void LoadingScreenController::OnCreate() {
        // Optional: Trigger async loads here if needed
    }

    void LoadingScreenController::OnUpdate() {
        // 1. Get Systems (Fix: Removed '&' because GetSystems() returns by value)
        auto systems = GameManager::GetInstance().GetSystems();

        // Safety checks
        if (!systems.resourceManager_ || !systems.ecs_) return;

        // 2. Create wrappers
        ResourceManager& resourceManager = *systems.resourceManager_;
        // Use EntityManager wrapper to access private storage data safely
        EntityManager entityMgr(*systems.ecs_);

        // 3. Get Loading Progress
        float progress = resourceManager.GetLoadingProgress();

        // 4. Update Text Component
        Entity myEntity = GetEntity();

        // Use EntityManager API instead of accessing ecs.entitySignatures directy
        if (entityMgr.HasComponent(myEntity, TextComponentType)) {

            // Use EntityManager API to get the component reference
            auto& textComp = entityMgr.GetComponent<TextComponent>(myEntity);

            std::stringstream ss;
            ss << "Loading... " << std::fixed << std::setprecision(0) << (progress * 100.0f) << "%";
            textComp.textToDraw = ss.str();
            textComp.isDirty = true;
        }

        // 5. Transition when done
        if (resourceManager.IsLoadingComplete()) {
            std::string nextScene = GameManager::GetInstance().GetNextScene();
			//Log when loading is complete
			Logger::Get().Info(DebugType::GameManager) << "Loading complete. Transitioning to next scene: " << nextScene << std::endl;

            if (!nextScene.empty()) {
                SceneManager::GetInstance().LoadScene(nextScene);
                GameManager::GetInstance().SetNextScene("");
            }
        }
    }

    void LoadingScreenController::OnDestroy() {
        // Cleanup if necessary
    }
}