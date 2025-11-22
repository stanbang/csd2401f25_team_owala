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

		std::string SerializeEntityToString(Entity entity);
	public:
		explicit HistoryManager(Framework::EntityStorage& ecs);

		/* Config */
		void SetUndoLimit(int limit);

		/* APIs */
		void ClearHistory();
		void Record(OpType opType, Entity entity, std::string description);
		void Push(const Operation& op);
		bool CanUndo() const;
		bool CanRedo() const;
		void Undo();
		void Redo();
	};
}