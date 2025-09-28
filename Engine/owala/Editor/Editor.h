#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include "EntityStorage.h"

namespace Framework {
	class GameManager; // Forward Declaration
	class CoreEngine; // Forward Declaration

	struct SystemTime; // Forward Declaration
}

namespace Editor {
	struct EditorSettings {
		bool showDebugInfo = true;
		bool showLevelEditor = true;
	};

	class Editor : public Framework::ISystem {
	private:
		Framework::CoreEngine* engine_ = nullptr; // Enforce that the Editor must be instantiated after engine
		Framework::GameManager* gameManager_ = nullptr; // Enforce that the Editor must be instantiated after engine

		// Internal Members
		bool initialized = false;
		//  Members for the system profiler display
		float profilerUpdateTimer_ = 0.0f;
		std::vector<Framework::SystemTime> profilerDisplayData_;
		float deltatime = 0.f;

		// Entity Inspector
		int selectedEntityIndex = 0;
		std::vector<std::string> componentNames;
		std::vector<const char*> componentItems;
		std::vector<Framework::ComponentType> componentTypes;
		int selectedComponentIndex = 0;
		std::vector<std::string> tagNames;
		std::vector<const char*> tagItems;
		std::vector<Framework::ComponentType> tagTypes;
		int selectedTagIndex = 0;

		// Internal Checkers
		void SanityCheck();

		// Internal Panels
		void ShowPerformanceProfile();
		void ShowLevelEditor();
		void ShowPlayerInspector();
		void ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent);
	public:
		bool isEnabled = true;
		EditorSettings settings;

		/* Constructors */
		// Editors should not be cloneable or assignable. Only 1 should ever exist on the engine.
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager);

		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override;
	};
}