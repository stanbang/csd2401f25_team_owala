/*****************************************************************//**
* \file   History.cpp
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file defines the HistoryManager class. History is a Editor-Only
*			system. The History system stores ECS changes in the Editor to the
*			Undo stack. Undo operation recording API is defined here.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "History.h"
#include "EntityManager.h"
#include "EntitySerializer.h"
#include "GameManager.h"

// Debugging
#include "Logger.h"

namespace Editor {
	using namespace Framework;

	void HistoryManager::Apply(const Operation& op, bool isUndo) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		const std::string& dataToUse = isUndo ? op.data.memento.beforeJSON : op.data.memento.afterJSON;
		std::cout << "Before Data: " << op.data.memento.beforeJSON << std::endl;
		std::cout << "After Data: " << op.data.memento.afterJSON << std::endl;
		std::cout << "Undo or Redo?: " << (isUndo ? "Undo" : "Redo") << std::endl;
		std::cout << op.description << std::endl;

		switch (op.opType) {
		case OpType::Create:
			if (isUndo) {
				entityMgr.DestroyEntity(op.data.entity);
			}
			else {
				if (op.data.mode == PayloadMode::Entity) {
					RestoreEntityFromSnapshot(op.data.entity, dataToUse);
				}
			}
			break;
		case OpType::Delete:
			if (isUndo) {
				RestoreEntityFromSnapshot(op.data.entity, dataToUse);
			}
			else {
				entityMgr.DestroyEntity(op.data.entity);
			}
			break;
		case OpType::ModifyComponent:
			ApplyComponentSnapshot(op.data.entity, op.data.componentType, dataToUse);
			break;
		default:
			break;
		}
	}

	void HistoryManager::RestoreEntityFromSnapshot(Entity entity, const std::string& json) {
		EntityStorage* ecs_ = &GameManager::GetInstance().GetECS();
		EntityManager entityMgr(*ecs_);

		// Empty JSON means "entity does not exist". Destroy entity instead.
		if (json.empty()) {
			std::cout << "HEY!!!" << std::endl;
			entityMgr.DestroyEntity(entity);
			return;
		}

		entityMgr.CreateEntityWithID(entity);

		rapidjson::Document doc;
		doc.Parse(json.c_str());
		if (doc.HasParseError()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to parse entity data from Memento!" << std::endl;
			return;
		}

		EntitySerializer::DeserializeEntityToExisting(*ecs_, entity, doc);
		SyncSceneLoadedState(entity);
	}

	void HistoryManager::ApplyComponentSnapshot(Entity entity, ComponentType type, const std::string& json) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		// Empty JSON means "component does not exist". Remove component instead.
		if (json.empty()) {
			if (entityMgr.IsTagComponent(type)) { // Empty Tags JSON means entity does not have any tags. Clear all tags.
				entityMgr.ClearTags(entity);
			}
			else {
				if (entityMgr.HasComponent(entity, type)) {
					entityMgr.RemoveComponent(entity, type);
				}
			}
			return;
		}

		rapidjson::Document doc;
		doc.Parse(json.c_str());
		if (doc.HasParseError()) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Failed to parse component data from Memento!" << std::endl;
			return;
		}

		// Special Case for Entity Properties
		if (type == ComponentType::MAX_COMPONENTS) { // Using MAX_COMPONENTS for Entity Properties, its intentional.
			EntityProperties& properties = entityMgr.GetProperties(entity);

			EntitySerializer::DeserializeEntityProperties(doc, properties);
			return;
		}

		// Special Case for Tags
		if (entityMgr.IsTagComponent(type)) {
			entityMgr.ClearTags(entity);

			if (!doc.HasMember(Serialization::TAGS) || !doc[Serialization::TAGS].IsArray()) {
				Framework::Logger::Info(DebugType::Editor) << "Tags data is malformed." << std::endl;
				return;
			}

			EntitySerializer::DeserializeTags(entityMgr, entity, doc);
			SyncSceneLoadedState(entity);
			return;
		}

		switch (type) {
			case ComponentType::TransformType: {
				Transform newTransform;
				if (EntitySerializer::DeserializeComponent(doc, newTransform)) {
					if (entityMgr.HasComponent(entity, ComponentType::TransformType)) {
						entityMgr.RemoveComponent(entity, ComponentType::TransformType);
					}
					entityMgr.AddComponent<Transform>(entity, newTransform);
				}
				break;
			}
			case ComponentType::VelocityType: {
				Velocity newVelocity;
				if (EntitySerializer::DeserializeComponent(doc, newVelocity)) {
					if (entityMgr.HasComponent(entity, ComponentType::VelocityType)) {
						entityMgr.RemoveComponent(entity, ComponentType::VelocityType);
					}
					entityMgr.AddComponent<Velocity>(entity, newVelocity);
				}
				break;
			}
			case ComponentType::AccelerationType: {
				Acceleration newAcceleration;
				if (EntitySerializer::DeserializeComponent(doc, newAcceleration)) {
					if (entityMgr.HasComponent(entity, ComponentType::AccelerationType)) {
						entityMgr.RemoveComponent(entity, ComponentType::AccelerationType);
					}
					entityMgr.AddComponent<Acceleration>(entity, newAcceleration);
				}
				break;
			}
			case ComponentType::TriangleType: {
				Triangle newTriangle;
				if (EntitySerializer::DeserializeComponent(doc, newTriangle)) {
					if (entityMgr.HasComponent(entity, ComponentType::TriangleType)) {
						entityMgr.RemoveComponent(entity, ComponentType::TriangleType);
					}
					entityMgr.AddComponent<Triangle>(entity, newTriangle);
				}
				break;
			}
			case ComponentType::RigidBodyType: {
				RigidBody newRigidBody;
				if (EntitySerializer::DeserializeComponent(doc, newRigidBody)) {
					if (entityMgr.HasComponent(entity, ComponentType::RigidBodyType)) {
						entityMgr.RemoveComponent(entity, ComponentType::RigidBodyType);
					}
					entityMgr.AddComponent<RigidBody>(entity, newRigidBody);
				}
				break;
			}
			case ComponentType::AABBColliderType: {
				AABBCollider newCollider;
				if (EntitySerializer::DeserializeComponent(doc, newCollider)) {
					if (entityMgr.HasComponent(entity, ComponentType::AABBColliderType)) {
						entityMgr.RemoveComponent(entity, ComponentType::AABBColliderType);
					}
					entityMgr.AddComponent<AABBCollider>(entity, newCollider);
				}
				break;
			}
			case ComponentType::PlayerStateType: {
				PlayerState newPlayerState;
				if (EntitySerializer::DeserializeComponent(doc, newPlayerState)) {
					if (entityMgr.HasComponent(entity, ComponentType::PlayerStateType)) {
						entityMgr.RemoveComponent(entity, ComponentType::PlayerStateType);
					}
					entityMgr.AddComponent<PlayerState>(entity, newPlayerState);
				}
				break;
			}
			case ComponentType::RotationFlagType: {
				RotationFlag newRotationFlag;
				if (EntitySerializer::DeserializeComponent(doc, newRotationFlag)) {
					if (entityMgr.HasComponent(entity, ComponentType::RotationFlagType)) {
						entityMgr.RemoveComponent(entity, ComponentType::RotationFlagType);
					}
					entityMgr.AddComponent<RotationFlag>(entity, newRotationFlag);
				}
				break;
			}
			case ComponentType::TrapPlatformComponentType: {
				TrapPlatformComponent newTrapPlatform;
				if (EntitySerializer::DeserializeComponent(doc, newTrapPlatform)) {
					if (entityMgr.HasComponent(entity, ComponentType::TrapPlatformComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::TrapPlatformComponentType);
					}
					entityMgr.AddComponent<TrapPlatformComponent>(entity, newTrapPlatform);
				}
				break;
			}
			case ComponentType::TextureComponentType: {
				TextureComponent newTextureComponent;
				if (EntitySerializer::DeserializeComponent(doc, newTextureComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::TextureComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::TextureComponentType);
					}
					entityMgr.AddComponent<TextureComponent>(entity, newTextureComponent);
				}
				break;
			}
			case ComponentType::AudioComponentType: {
				AudioComponent newAudioComponent;
				if (EntitySerializer::DeserializeComponent(doc, newAudioComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::AudioComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::AudioComponentType);
					}
					entityMgr.AddComponent<AudioComponent>(entity, newAudioComponent);
				}
				break;
			}
			case ComponentType::AnimationComponentType: {
				AnimationComponent newAnimationComponent;
				if (EntitySerializer::DeserializeComponent(doc, newAnimationComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::AnimationComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::AnimationComponentType);
					}
					entityMgr.AddComponent<AnimationComponent>(entity, newAnimationComponent);
				}
				break;
			}
			case ComponentType::TextComponentType: {
				TextComponent newTextComponent;
				if (EntitySerializer::DeserializeComponent(doc, newTextComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::TextComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::TextComponentType);
					}
					entityMgr.AddComponent<TextComponent>(entity, newTextComponent);
				}
				break;
			}
			case ComponentType::CameraType: {
				Camera newCamera;
				if (EntitySerializer::DeserializeComponent(doc, newCamera)) {
					if (entityMgr.HasComponent(entity, ComponentType::CameraType)) {
						entityMgr.RemoveComponent(entity, ComponentType::CameraType);
					}
					entityMgr.AddComponent<Camera>(entity, newCamera);
				}
				break;
			}
			case ComponentType::NativeScriptsComponentType: {
				NativeScriptsComponent newNativeScriptsComponent;
				if (EntitySerializer::DeserializeComponent(doc, newNativeScriptsComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::NativeScriptsComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::NativeScriptsComponentType);
					}
					entityMgr.AddComponent<NativeScriptsComponent>(entity, std::move(newNativeScriptsComponent));
				}
				break;
			}
			case ComponentType::ButtonType: {
				Button newButton;
				if (EntitySerializer::DeserializeComponent(doc, newButton)) {
					if (entityMgr.HasComponent(entity, ComponentType::ButtonType)) {
						entityMgr.RemoveComponent(entity, ComponentType::ButtonType);
					}
					entityMgr.AddComponent<Button>(entity, newButton);
				}
				break;
			}
			case ComponentType::SceneOwnedType: {
				SceneOwned newSceneOwned;
				if (EntitySerializer::DeserializeComponent(doc, newSceneOwned)) {
					if (entityMgr.HasComponent(entity, ComponentType::SceneOwnedType)) {
						entityMgr.RemoveComponent(entity, ComponentType::SceneOwnedType);
					}
					entityMgr.AddComponent<SceneOwned>(entity, newSceneOwned);
					SyncSceneLoadedState(entity);
				}
				break;
			}
		
			case ComponentType::FSMComponentType:
			{
				FSMComponent newFSM;
				if (EntitySerializer::DeserializeComponent(doc, newFSM)) {
					if (entityMgr.HasComponent(entity, ComponentType::FSMComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::FSMComponentType);
					}
					entityMgr.AddComponent<FSMComponent>(entity, newFSM);
				}
				break;
			}

			case ComponentType::ParticleComponentType: {
				ParticleComponent newParticleComponent;
				if (entityMgr.HasComponent(entity, ComponentType::ParticleComponentType)) {
					entityMgr.RemoveComponent(entity, ComponentType::ParticleComponentType);
				}
				entityMgr.AddComponent<ParticleComponent>(entity, newParticleComponent);
				break;
			}

			case ComponentType::ParticleEmitterComponentType: {
				ParticleEmitterComponent newParticleEmitterComponent;
				if (EntitySerializer::DeserializeComponent(doc, newParticleEmitterComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::ParticleEmitterComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::ParticleEmitterComponentType);
					}
					entityMgr.AddComponent<ParticleEmitterComponent>(entity, newParticleEmitterComponent);
				}
				break;
			}
			case ComponentType::SliderType: {
				Slider newSlider;
				if (EntitySerializer::DeserializeComponent(doc, newSlider)) {
					if (entityMgr.HasComponent(entity, ComponentType::SliderType)) {
						entityMgr.RemoveComponent(entity, ComponentType::SliderType);
					}
					entityMgr.AddComponent<Slider>(entity, newSlider);
				}
				break;
			}
			case ComponentType::PlayerHealthComponentType: {
				PlayerHealthComponent newPlayerHealth;
				if (EntitySerializer::DeserializeComponent(doc, newPlayerHealth)) {
					if (entityMgr.HasComponent(entity, ComponentType::PlayerHealthComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::PlayerHealthComponentType);
					}
					entityMgr.AddComponent<PlayerHealthComponent>(entity, newPlayerHealth);
				}
				break;
			}
			default:
				Logger::Info(DebugType::Editor) << "HistoryManager::ApplyComponentSnapshot: Unsupported component type. No Changes applied" << std::endl;
		}
	}

	void HistoryManager::PruneHistoryToLimit() {
		if (maxHistorySteps <= 0) {
			Logger::Info(DebugType::Editor) << "HistoryManager::Prune: Max steps less than 0. Setting to default." << std::endl;
			maxHistorySteps = 20;
		}

		if (undoStack.empty()) return;

		int steps = 0;
		std::size_t index = undoStack.size();

		// Calculate Number of History Steps consumed
		while (index > 0 && steps < maxHistorySteps) {
			const Operation& op = undoStack[index - 1];

			if (op.groupID == 0) { // Single Operation
				++steps;
				--index;
			}
			else { // Operation belongs to a group.
				const int groupID = op.groupID;
				++steps;

				while (index > 0 && undoStack[index - 1].groupID == groupID) {
					--index;
				}
			}
		}

		// Prune everything after recorded index.
		if (index > 0) {
			undoStack.erase(undoStack.begin(), undoStack.begin() + index);
		}
	}

	void HistoryManager::SyncSceneLoadedState(Entity entity) {
		using namespace Framework;
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		SceneManager& sceneMgr = SceneManager::GetInstance();

		if (entityMgr.HasComponent(entity, SceneOwnedType)) {
			const SceneOwned& owned = entityMgr.GetComponent<SceneOwned>(entity);
			sceneMgr.AddEntityToScene(owned.sceneIndex, entity);
		}
		else {
			const SceneManagement::SceneAsset& activeScene = sceneMgr.GetActiveScene();
			if (activeScene.IsValid()) {
				sceneMgr.AddEntityToScene(activeScene, entity);
			}
		}
	}

	// ---------------------------------------------------------------------------
	// Configuration
	// ---------------------------------------------------------------------------
	void HistoryManager::SetUndoLimit(int limit) {
		if (limit <= 0) {
			return;
		}

		maxHistorySteps = limit;

		PruneHistoryToLimit();
	}

	// ---------------------------------------------------------------------------
	// Accessors
	// ---------------------------------------------------------------------------
	std::string HistoryManager::GetLatestUndoDescription() const {
		if (undoStack.empty()) {
			return {};
		}
		return undoStack.back().description;
	}

	std::string HistoryManager::GetLatestRedoDescription() const {
		if (redoStack.empty()) {
			return {};
		}
		return redoStack.back().description;
	}

	std::vector<std::string> HistoryManager::GetUndoHistoryDescriptions() const {
		std::vector<std::string> descriptions;
		descriptions.reserve(undoStack.size());

		for (const Operation& operation : undoStack) {
			descriptions.push_back(operation.description);
		}

		return descriptions;
	}

	std::vector<std::string> HistoryManager::GetRedoHistoryDescriptions() const {
		std::vector<std::string> descriptions;
		descriptions.reserve(redoStack.size());

		for (const Operation& operation : redoStack) {
			descriptions.push_back(operation.description);
		}

		return descriptions;
	}

	// ---------------------------------------------------------------------------
	// APIs
	// ---------------------------------------------------------------------------
	void HistoryManager::ClearHistory() {
		undoStack.clear();
		redoStack.clear();
	}

	void HistoryManager::Record(OpType opType, Entity entity, const std::string& description) {
		// Do not record any operations during Play Mode.
		if (GameManager::GetInstance().GetEditor()->IsGamePlaying()) return;

		Operation op;
		EntityStorage& ecs_ = GameManager::GetInstance().GetECS();

		op.opType = opType;
		op.description = description;
		op.data.entity = entity;

		switch (opType) {
		case OpType::Create:
			op.data.mode = PayloadMode::Entity;
			op.data.memento.beforeJSON.clear();
			op.data.memento.afterJSON = EntitySerializer::SerializeEntityToString(entity, ecs_);
			break;
		case OpType::Delete:
			op.data.mode = PayloadMode::Entity;
			op.data.memento.beforeJSON = EntitySerializer::SerializeEntityToString(entity, ecs_);
			op.data.memento.afterJSON.clear();
			break;
		default: // Components should be handled via the dedicated function.
			return;
		}

		Push(op);
	}

	void HistoryManager::RecordModifyComponent(Entity entity, const std::string& description, const std::string& beforeJSON, const std::string& afterJSON, ComponentType compType) {
		// Do not record any operations during Play Mode.
		if (GameManager::GetInstance().GetEditor()->IsGamePlaying()) return;

		Operation op;

		op.opType = OpType::ModifyComponent;
		op.description = description;
		op.data.entity = entity;
		op.data.componentType = compType;

		op.data.mode = PayloadMode::Component;
		op.data.memento.beforeJSON = beforeJSON;
		op.data.memento.afterJSON = afterJSON;

		Push(op);
	}

	void HistoryManager::Push(Operation& op) {
		if (op.opType == OpType::None) {
			Logger::Info(DebugType::Editor) << "HistoryManager::Push: Invalid operation type, cannot push to history." << std::endl;
			return;
		}

		// If operation being pushed is currently being recorded as a grouped operation
		if (activeGroupID != 0) {
			op.groupID = activeGroupID;
		}

		undoStack.push_back(op);
		redoStack.clear();

		PruneHistoryToLimit();
	}

	bool HistoryManager::CanUndo() const {
		return !undoStack.empty();
	}

	bool HistoryManager::CanRedo() const {
		return !redoStack.empty();
	}

	void HistoryManager::Undo() {
		if (!CanUndo()) {
			return;
		}

		int groupID = undoStack.back().groupID;

		// If operation does not belong to a group
		if (groupID == 0) {
			const Operation op = undoStack.back(); // Get the latest operation
			Apply(op, true); // Apply the operation as an undo
			undoStack.pop_back(); // Remove it from the undo stack
			redoStack.push_back(op); // Push it to the redo stack
			return;
		}

		// Grouped Operations
		std::vector<Operation> groupOps;
		while (!undoStack.empty() && undoStack.back().groupID == groupID) {
			Operation op = undoStack.back();
			undoStack.pop_back();

			Apply(op, true);
			groupOps.push_back(std::move(op));
		}

		for (std::vector<Operation>::reverse_iterator it = groupOps.rbegin(); it != groupOps.rend(); ++it) {
			redoStack.push_back(*it);
		}
	}

	void HistoryManager::Redo() {
		if (!CanRedo()) {
			return;
		}

		int groupID = redoStack.back().groupID;

		// If operation does not belong to a group
		if (groupID == 0) {
			const Operation op = redoStack.back(); // Get the operation preceding the last undone operation
			Apply(op, false); // Apply the operation as a redo
			redoStack.pop_back(); // Remove it from the redo stack
			undoStack.push_back(op); // Push it to the undo stack
			return;
		}

		// Grouped Operations
		std::vector<Operation> groupOps;
		while (!redoStack.empty() && redoStack.back().groupID == groupID) {
			groupOps.push_back(redoStack.back());
			redoStack.pop_back();
		}

		// Reapply in reverse order
		for (std::vector<Operation>::reverse_iterator it = groupOps.rbegin(); it != groupOps.rend(); ++it) {
			Apply(*it, false);
			undoStack.push_back(*it);
		}
	}

	// ---------------------------------------------------------------------------
	// Grouping APIs
	// ---------------------------------------------------------------------------
	void HistoryManager::BeginGroup() {
		if (groupDepth == 0) {
			activeGroupID = nextGroupID++;
		}
		++groupDepth;
	}

	void HistoryManager::EndGroup() {
		if (groupDepth == 0) {
			Logger::Info(DebugType::Editor) << "History::EndGroup: Unmatched Endgroup! Did you forget to call BeginGroup?" << std::endl;
			return;
		}

		--groupDepth;
		if (groupDepth == 0) {
			activeGroupID = 0;
		}
	}

	bool HistoryManager::IsGroupActive() const noexcept {
		return activeGroupID != 0;
	}
}