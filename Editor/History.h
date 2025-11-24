#pragma once
// Entity
#include "Entity.h"
#include "EntityStorage.h"
// Math
#include "Vector2D.h"

// Standard Library
#include <string>
#include <vector>

namespace Editor{
	enum class OpType : uint8_t {
		None = 0,
		Create,
		Delete,
		ModifyComponent,
	};

	enum class PayloadMode : uint8_t {
		Component,
		Entity
	};

	struct Memento {
		std::string beforeJSON;
		std::string afterJSON;
	};

	struct OpData {
		Entity entity;
		PayloadMode mode = PayloadMode::Component;
		Framework::ComponentType componentType = Framework::ComponentType::MAX_COMPONENTS;
		Memento memento;
	};

	struct Operation {
		OpType opType = OpType::None;
		std::string description;

		OpData data;
	};

	class HistoryManager {
	private:
		/* Preservation */
		Framework::EntityStorage* ecs_ = nullptr;
		std::vector<Operation> undoStack;
		std::vector<Operation> redoStack;
		int maxHistorySteps{ 20 };

		/* Internal Helpers */
		void PruneHistoryToLimit();
		void Apply(const Operation& op, bool isUndo);
		void RestoreEntityFromSnapshot(Entity entity, const std::string& json);
		void ApplyComponentSnapshot(Entity entity, Framework::ComponentType type, const std::string& json);
	public:
		explicit HistoryManager(Framework::EntityStorage& ecs);

		/* Config */
		void SetUndoLimit(int limit);

		/* Accessors */
		std::string GetLatestUndoDescription() const;
		std::string GetLatestRedoDescription() const;
		std::vector<std::string> GetUndoHistoryDescriptions() const;
		std::vector<std::string> GetRedoHistoryDescriptions() const;

		/* APIs */
		void ClearHistory();
		// Function for recording Entity-Level operations.
		void Record(OpType opType, Entity entity, const std::string& description);
		// Dedicated Function for recording operations relating to Component Modifications.
		// Add/Remove/Modify Components should use this.
		void RecordModifyComponent(Entity entity, const std::string& description, const std::string& beforeJSON, const std::string& afterJSON, Framework::ComponentType compType = Framework::ComponentType::MAX_COMPONENTS);
		void Push(const Operation& op);
		bool CanUndo() const;
		bool CanRedo() const;
		void Undo();
		void Redo();
	};
}