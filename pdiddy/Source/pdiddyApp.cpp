#include "Core/Application.h"
#include "Core/Core.h"
#include "Managers/GameManager/GameManager.h"

namespace Framework {

    class pdiddyApp : public Application {
    public:
        pdiddyApp(CoreEngine& engine) : m_Engine(engine) {}

        void Initialize() override {
            // TODO: register game systems and load the first scene
        }

        void Update() override {}

        void Shutdown() override {}

    private:
        CoreEngine& m_Engine;
    };

    Application* CreateApplication(CoreEngine& engine) {
        return new pdiddyApp(engine);
    }

} // namespace Framework
