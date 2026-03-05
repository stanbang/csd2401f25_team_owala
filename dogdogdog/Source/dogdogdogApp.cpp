#include "Core/Application.h"
#include "Core/Core.h"
#include "Managers/GameManager/GameManager.h"

namespace Framework {

    class dogdogdogApp : public Application {
    public:
        dogdogdogApp(CoreEngine& engine) : m_Engine(engine) {}

        void Initialize() override {
            // TODO: register game systems and load the first scene
        }

        void Update() override {}

        void Shutdown() override {}

    private:
        CoreEngine& m_Engine;
    };

    Application* CreateApplication(CoreEngine& engine) {
        return new dogdogdogApp(engine);
    }

} // namespace Framework
