/*****************************************************************//**
* \file   ScriptableEntity.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file defines the base class for script behaviors.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Entity.h"

namespace Framework {

	// Base class for Script Behaviors. Technically also a wrapper.
	class ScriptableEntity {
	private:
		Entity entity_ = 0;

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

		void InvokeOnDestroy() { OnDestroy(); }
	};
}