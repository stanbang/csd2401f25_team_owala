#include "History.h"
#include "EntityManager.h"
#include "EntitySerializer.h"
#include "SerializationKeys.h"
#include "ScriptRegistry.h"

// Debugging
#include "Logger.h"

namespace Editor {
	using namespace Framework;
	HistoryManager::HistoryManager(Framework::EntityStorage& ecs) : ecs_(&ecs) {}

	void HistoryManager::Apply(const Operation& op, bool isUndo) {
		if (!ecs_) {
			Logger::Info(DebugType::Editor) << "HistoryManager::Apply: ECS is null, cannot apply operation." << std::endl;
			return;
		}

		EntityManager entityMgr(*ecs_);
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
	}

	void HistoryManager::ApplyComponentSnapshot(Entity entity, ComponentType type, const std::string& json) {
		EntityManager entityMgr(*ecs_);

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
			return;
		}

		switch (type) {
			case ComponentType::PositionType: {
				Position newPosition;
				if (EntitySerializer::DeserializeComponent(doc, newPosition)) {
					if (entityMgr.HasComponent(entity, ComponentType::PositionType)) {
						entityMgr.RemoveComponent(entity, ComponentType::PositionType);
					}
					entityMgr.AddComponent<Position>(entity, newPosition);
				}
				break;
			}
			case ComponentType::SpawnPointType: {
				SpawnPoint newSpawnPoint;
				if (EntitySerializer::DeserializeComponent(doc, newSpawnPoint)) {
					if (entityMgr.HasComponent(entity, ComponentType::SpawnPointType)) {
						entityMgr.RemoveComponent(entity, ComponentType::SpawnPointType);
					}
					entityMgr.AddComponent<SpawnPoint>(entity, newSpawnPoint);
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
			case ComponentType::ScalingPlatformComponentType: {
				ScalingPlatformComponent newScalingPlatform;
				if (EntitySerializer::DeserializeComponent(doc, newScalingPlatform)) {
					if (entityMgr.HasComponent(entity, ComponentType::ScalingPlatformComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::ScalingPlatformComponentType);
					}
					entityMgr.AddComponent<ScalingPlatformComponent>(entity, newScalingPlatform);
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
				}
				break;
			}
			case ComponentType::BirdAIComponentType: {
				BirdAIComponent newBirdAIComponent;
				if (EntitySerializer::DeserializeComponent(doc, newBirdAIComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::BirdAIComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::BirdAIComponentType);
					}
					entityMgr.AddComponent<BirdAIComponent>(entity, newBirdAIComponent);
				}
				break;
			}
			case ComponentType::SkeletonAIComponentType: {
				SkeletonAIComponent newSkeletonAIComponent;
				if (EntitySerializer::DeserializeComponent(doc, newSkeletonAIComponent)) {
					if (entityMgr.HasComponent(entity, ComponentType::SkeletonAIComponentType)) {
						entityMgr.RemoveComponent(entity, ComponentType::SkeletonAIComponentType);
					}
					entityMgr.AddComponent<SkeletonAIComponent>(entity, newSkeletonAIComponent);
				}
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
			case ComponentType::RotationType: {
				Rotation newRotationComp;
				if (EntitySerializer::DeserializeComponent(doc, newRotationComp)) {
					if (entityMgr.HasComponent(entity, ComponentType::RotationType)) {
						entityMgr.RemoveComponent(entity, ComponentType::RotationType);
					}
					entityMgr.AddComponent<Rotation>(entity, newRotationComp);
				}
				break;
			}
			case ComponentType::ScaleType: {
				Scale newScaleComp;
				if (EntitySerializer::DeserializeComponent(doc, newScaleComp)) {
					if (entityMgr.HasComponent(entity, ComponentType::ScaleType)) {
						entityMgr.RemoveComponent(entity, ComponentType::ScaleType);
					}
					entityMgr.AddComponent<Scale>(entity, newScaleComp);
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

		if (undoStack.size() > static_cast<size_t>(maxHistorySteps)) {
			undoStack.erase(undoStack.begin(), undoStack.begin() + (undoStack.size() - maxHistorySteps));
		}

		if (redoStack.size() > static_cast<size_t>(maxHistorySteps)) {
			redoStack.erase(redoStack.begin(), redoStack.begin() + (redoStack.size() - maxHistorySteps));
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
		Operation op;

		op.opType = opType;
		op.description = description;
		op.data.entity = entity;

		switch (opType) {
		case OpType::Create:
			op.data.mode = PayloadMode::Entity;
			op.data.memento.beforeJSON.clear();
			op.data.memento.afterJSON = EntitySerializer::SerializeEntityToString(entity, *ecs_);
			break;
		case OpType::Delete:
			op.data.mode = PayloadMode::Entity;
			op.data.memento.beforeJSON = EntitySerializer::SerializeEntityToString(entity, *ecs_);
			op.data.memento.afterJSON.clear();
			break;
		default:
			return;
		}

		Push(op);
	}

	void HistoryManager::RecordModifyComponent(Entity entity, const std::string& description, const std::string& beforeJSON, const std::string& afterJSON, ComponentType compType) {
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

	void HistoryManager::Push(const Operation& op) {
		if (!ecs_) {
			Logger::Info(DebugType::Editor) << "HistoryManager::Push: ECS is null, cannot push to history." << std::endl;
			return;
		}

		if (op.opType == OpType::None) {
			Logger::Info(DebugType::Editor) << "HistoryManager::Push: Invalid operation type, cannot push to history." << std::endl;
			return;
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

		const Operation op = undoStack.back(); // Get the latest operation
		Apply(op, true); // Apply the operation as an undo
		undoStack.pop_back(); // Remove it from the undo stack
		redoStack.push_back(op); // Push it to the redo stack
	}

	void HistoryManager::Redo() {
		if (!CanRedo()) {
			return;
		}

		const Operation op = redoStack.back(); // Get the operation preceding the last undone operation
		Apply(op, false); // Apply the operation as a redo
		redoStack.pop_back(); // Remove it from the redo stack
		undoStack.push_back(op); // Push it to the undo stack
	}
}