#pragma once
#include "ScriptableEntity.h"
#include <string>

using namespace Framework;

// -----------------------------------------------------------------------------
// StageboundRunSubmitter  (Game layer - Stagebound specific)
//
// Attach ONE instance of this script to a persistent entity that lives for the
// entire game session (e.g. your GameManager entity or a dedicated "Services"
// entity that is never destroyed between scenes).
//
// Responsibilities:
//   - Configures FirebaseManager with the project URL at startup.
//   - Registers the OnRunFinished callback on TimerSystem.
//   - When a run finishes, checks GameStateFlags::shouldRecordTime and the
//     player name, then decides whether to submit to Firebase.
//
// This is the ONLY file in your game that needs to know about:
//   - The Firebase database URL
//   - GameStateFlags
//   - PlayerNameManager
//
// The engine-layer files (TimerSystem, FirebaseManager, trigger scripts) have
// no knowledge of any of this.
// -----------------------------------------------------------------------------
class StageboundRunSubmitter : public ScriptableEntity {
protected:
    void OnCreate()  override;
    void OnDestroy() override;
    // No per-frame work needed - the callback fires from TimerSystem::Stop()
    void OnUpdate()  override {}
};
