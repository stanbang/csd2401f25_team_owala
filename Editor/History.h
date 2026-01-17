/*****************************************************************//**
* \file   History.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file declares the HistoryManager class and other structures associated 
*			to the History (Undo/Redo System). History is a Editor-Only
*			system. The History system stores ECS changes in the Editor to the
*			Undo stack.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
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
		Entity entity = 0;
		PayloadMode mode = PayloadMode::Component;
		Framework::ComponentType componentType = Framework::ComponentType::MAX_COMPONENTS;
		Memento memento;
	};

	struct Operation {
		OpType opType = OpType::None;
		std::string description;
		int groupID = 0;

		OpData data;
	};

	class HistoryManager {
	private:
		/* Preservation */
		std::vector<Operation> undoStack; // Current operations that can be undone.
		std::vector<Operation> redoStack; // Current operations that can be redone.
		int maxHistorySteps{ 20 }; // Maximum stack size of the Undo Stack.

		// Might convert grouping system to vector of vector of operations in the future for better readability, see how - Jason.
		/* Grouping States */
		int nextGroupID = 1;
		int activeGroupID = 0;
		int groupDepth = 0;

		/* Internal Helpers */
		// Prunes the Undo Stack to the limit, starting from the oldest operation in the Undo Stack.
		void PruneHistoryToLimit();
		// The actual application of the Undo/Redo operation.
		void Apply(const Operation& op, bool isUndo);
		// Restores an entity to the state recorded in the JSON snapshot.
		void RestoreEntityFromSnapshot(Entity entity, const std::string& json);
		// Restores a component to the state recorded in the JSON snapshot.
		void ApplyComponentSnapshot(Entity entity, Framework::ComponentType type, const std::string& json);
	public:
		HistoryManager() = default;

		/* Config */
		// Sets the max limit of the Undo Stack. Undo Stack will be pruned to fit the new limit.
		void SetUndoLimit(int limit);

		/* Accessors */
		// Retrieves the description of the latest operation in the Undo Stack.
		std::string GetLatestUndoDescription() const;
		// Retrieves the description of the latest operation in the Redo Stack.
		std::string GetLatestRedoDescription() const;
		// Retrieves the descriptions of all operations in the Undo Stack.
		std::vector<std::string> GetUndoHistoryDescriptions() const;
		// Retrieves the descriptions of all operations in the Redo Stack.
		std::vector<std::string> GetRedoHistoryDescriptions() const;

		/* APIs */
		void ClearHistory();
		// Function for recording Entity-Level operations.
		void Record(OpType opType, Entity entity, const std::string& description);
		// Dedicated Function for recording operations relating to Component Modifications.
		// Add/Remove/Modify Components should use this.
		void RecordModifyComponent(Entity entity, const std::string& description, const std::string& beforeJSON, const std::string& afterJSON, Framework::ComponentType compType = Framework::ComponentType::MAX_COMPONENTS);
		// Pushes an operation to the Undo Stack. Do not use this unless you know what you are doing.
		void Push(Operation& op);
		// Returns true if there is at least 1 operation in the Undo Stack.
		bool CanUndo() const;
		// Returns true if there is at least 1 operation in the Redo Stack.
		bool CanRedo() const;
		// Perform an Undo operation.
		void Undo();
		// Perform an Redo operation.
		void Redo();

		/* Grouping APIs */
		// Begins appending recorded operations following this call into a group.
		void BeginGroup();
		// Combines all operations that were recorded since the BeginGroup() call into a single operation and pushes it onto the Undo Stack.
		void EndGroup();
		// Returns true if operations are currently being recorded as a grouped operation.
		bool IsGroupActive() const noexcept;
	};
}