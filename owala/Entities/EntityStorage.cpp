/*****************************************************************//**
 * \file   EntityStorage.cpp
 *
 * \primary_author
 *      Jason Lou(60%)
 * \secondary_author
 *		Ng Jing Wen(40%)
 *
 * \brief	Contains the definition of EntityStorage class which stores
 *			unordered maps of entity-component pairs for all components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "EntityStorage.h"
#include "Components.h"

namespace Framework {

	/**
	 * Copy Constructor - Performs a deep copy of all components, including properly 
	 * handling the NativeScriptsComponent to ensure that script instances are not shared between EntityStorage copies.
	 * 
	 * \param other
	 */
	EntityStorage::EntityStorage(const EntityStorage& other) {
		entitySignatures = other.entitySignatures;

		properties = other.properties;
		transform = other.transform;
		velocity = other.velocity;
		acceleration = other.acceleration;
		triangle = other.triangle;
		rigidBody = other.rigidBody;
		colliders = other.colliders;
		playerState = other.playerState;
		rotationFlag = other.rotationFlag;
		trapPlatform = other.trapPlatform;
		button = other.button;
		tex = other.tex;
		audio = other.audio;
		animatedTex = other.animatedTex;
		text = other.text;
		cameras = other.cameras;
		fsmComponent = other.fsmComponent;
		particleComponent = other.particleComponent;
		particleEmitterComponent = other.particleEmitterComponent;
		sliders = other.sliders;
		sceneOwned = other.sceneOwned;
		playerHealth = other.playerHealth;


		// Copy NativeScriptsComponent with care to avoid sharing script instances
		for (const auto& [entity, scripts] : other.nativeScripts) {
			nativeScripts[entity] = scripts;
		}
	}

	/**
	 * Copy Assignment Operator - Performs a deep copy of all components, including properly
	 * handling the NativeScriptsComponent to ensure that script instances are not shared between EntityStorage copies.
	 * 
	 * \param other
	 * \return 
	 */
	EntityStorage& EntityStorage::operator=(const EntityStorage& other) {
		if (this == &other) return *this;

		// Reset all existing NativeScript instances before copying new data
		for (auto& [entity, scripts] : nativeScripts) {
			for (NativeScriptsComponent::NativeScript& script : scripts.scripts) {
				script.Reset();
			}
		}

		entitySignatures = other.entitySignatures;

		properties = other.properties;
		transform = other.transform;
		velocity = other.velocity;
		acceleration = other.acceleration;
		triangle = other.triangle;
		rigidBody = other.rigidBody;
		colliders = other.colliders;
		playerState = other.playerState;
		rotationFlag = other.rotationFlag;
		trapPlatform = other.trapPlatform;
		button = other.button;
		tex = other.tex;
		audio = other.audio;
		animatedTex = other.animatedTex;
		text = other.text;
		cameras = other.cameras;
		fsmComponent = other.fsmComponent;
		particleComponent = other.particleComponent;
		particleEmitterComponent = other.particleEmitterComponent;
		sliders = other.sliders;
		sceneOwned = other.sceneOwned;
		playerHealth = other.playerHealth;

		nativeScripts.clear();

		// Copy NativeScriptsComponent with care to avoid sharing script instances
		for (const auto& [entity, scripts] : other.nativeScripts) {
			nativeScripts[entity] = scripts;
		}
		return *this;
	}

	EntityStorage::~EntityStorage() {
		for (auto& [entity, scripts] : nativeScripts) {
			for (auto& script : scripts.scripts) {
				script.Reset();
			}
		}
	}

	/**
	 * Swaps the contents of this EntityStorage with another. 
	 * This is a noexcept operation that efficiently exchanges all component data and 
	 * metadata between the two storages without copying, including properly handling the 
	 * NativeScriptsComponent to ensure that script instances are not shared after the swap.
	 * 
	 * \param other
	 * \return 
	 */
	void EntityStorage::SwapWith(EntityStorage& other) noexcept {
		// Metadata
		std::swap(entitySignatures, other.entitySignatures);
		std::swap(properties, other.properties);

		// Components
		std::swap(transform, other.transform);
		std::swap(velocity, other.velocity);
		std::swap(acceleration, other.acceleration);
		std::swap(triangle, other.triangle);
		std::swap(rigidBody, other.rigidBody);
		std::swap(colliders, other.colliders);
		std::swap(playerState, other.playerState);
		std::swap(rotationFlag, other.rotationFlag);
		std::swap(trapPlatform, other.trapPlatform);
		std::swap(button, other.button);
		std::swap(tex, other.tex);
		std::swap(audio, other.audio);
		std::swap(animatedTex, other.animatedTex);
		std::swap(text, other.text);
		std::swap(cameras, other.cameras);
		std::swap(fsmComponent, other.fsmComponent);
		std::swap(particleComponent, other.particleComponent);
		std::swap(particleEmitterComponent, other.particleEmitterComponent);
		std::swap(sliders, other.sliders);
		std::swap(sceneOwned, other.sceneOwned);
		std::swap(playerHealth, other.playerHealth);


		std::swap(nativeScripts, other.nativeScripts);
	}
}