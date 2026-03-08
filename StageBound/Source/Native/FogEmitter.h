/*****************************************************************//**
 * \file	FogEmitter.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author

 *
 * \brief   Script for emitting fog VFX in the level.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"
#include "Components.h"

using namespace Framework;

class FogEmitter : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;

private:
	struct FogParticle {
		Entity entity;
		float lifeTimer;
		float maxLife;
	};

	std::vector<FogParticle> activeFogs;
	float nextSpawnTimer;

	Vector2D fogTextureHalfSize = { 200.0f, 200.0f };
	float fogVelocity = 50.0f;
	float fogLifeSpan = 10.0f; // 10sec

	float minSpawnInterval = 4.0f;
	float maxSpawnInterval = 8.0f;
};
