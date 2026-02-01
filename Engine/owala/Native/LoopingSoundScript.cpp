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
