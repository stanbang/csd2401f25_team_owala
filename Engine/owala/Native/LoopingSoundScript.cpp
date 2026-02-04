/*****************************************************************//**
 * \file	LoopingSoundScript.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the Looping sound script.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "LoopingSoundScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "OwalaTime.h"

REGISTER_SCRIPT(LoopingSoundScript, "LoopingSoundScript")

float loopTime = 5.f;

void LoopingSoundScript::OnCreate() {
	// Initialization Code Here
}

void LoopingSoundScript::OnUpdate() {
    ResourceManager& rsm = *GameManager::GetInstance().GetSystems().resourceManager_;
    AudioSystem& audio = *GameManager::GetInstance().GetSystems().audio_;

    if (loopTime <= 0.f) {
        try {
            FMOD::Sound& sfx = rsm.get<FMOD::Sound>("soundeffect.ogg");
            audio.play(sfx, AudioType::SFX, 1.0f, false);
        }
        catch (...) {
            // ignore if missing
        }

        loopTime = 5.f;
    }
    else {
        loopTime -= Time::DeltaTime();
    }
}

void LoopingSoundScript::OnDestroy() {
	// Logic to be called before Entity is deleted
}
