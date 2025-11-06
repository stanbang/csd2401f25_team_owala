#pragma once
#include "Entity.h"

namespace Framework {

	// Wrapper class designating an Entity is a Scriptable Entity
	class ScriptableEntity {
	private:
		Entity entity_;

		friend class GameManager;
		friend class EntityManager;
	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate() {}
	public:
		virtual ~ScriptableEntity() {}

		void SetEntity(Entity entity) { entity_ = entity; }
		Entity GetEntity() const { return entity_; }
	};
}