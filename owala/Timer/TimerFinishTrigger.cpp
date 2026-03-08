#include "pch.h"
#include "TimerFinishTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"
#include "PlayerNameManager.h"
#include "GameStateFlags.h"

REGISTER_SCRIPT(TimerFinishTriggerScript, "TimerFinishTriggerScript")

void TimerFinishTriggerScript::OnCreate() {
    m_fired = false;
    Framework::Logger::Get().Info(DebugType::PlayerSystem) << "[TimerFinish] Created" << std::endl;
}

void TimerFinishTriggerScript::OnUpdate() {
    if (m_fired) {
        return;
    }

    if (!TimerSystem::Get().IsRunning()) {
        return;
    }

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    Entity self = GetEntity();

    std::vector<Entity> players = mgr.GetAllEntitiesWithComponent(false, PlayerTagType);
    if (players.empty()) return;
    Entity player = players[0];

    if (!mgr.HasComponent(player, AABBColliderType)) return;
    if (!mgr.HasComponent(self, AABBColliderType)) return;
    if (!mgr.HasComponent(player, TransformType))    return;
    if (!mgr.HasComponent(self, TransformType))    return;

    AABBCollider& playerCol = mgr.GetComponent<AABBCollider>(player);
    AABBCollider& triggerCol = mgr.GetComponent<AABBCollider>(self);
    Transform& playerTf = mgr.GetComponent<Transform>(player);
    Transform& selfTf = mgr.GetComponent<Transform>(self);

    AABB_Bounds playerBounds{
        { playerTf.position.x - playerCol.HalfSize.x + playerCol.Offset.x,
          playerTf.position.y - playerCol.HalfSize.y + playerCol.Offset.y },
        { playerTf.position.x + playerCol.HalfSize.x + playerCol.Offset.x,
          playerTf.position.y + playerCol.HalfSize.y + playerCol.Offset.y }
    };
    AABB_Bounds triggerBounds{
        { selfTf.position.x - triggerCol.HalfSize.x + triggerCol.Offset.x,
          selfTf.position.y - triggerCol.HalfSize.y + triggerCol.Offset.y },
        { selfTf.position.x + triggerCol.HalfSize.x + triggerCol.Offset.x,
          selfTf.position.y + triggerCol.HalfSize.y + triggerCol.Offset.y }
    };

    if (CollisionIntersection_RectRect_Static(playerBounds, triggerBounds)) {
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] *** PORTAL REACHED! Stopping timer... ***" << std::endl;

        float finalTime = TimerSystem::Get().GetElapsed();
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] Final time: " << finalTime << " seconds" << std::endl;

        // Log ALL relevant state
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] ===== DEBUG STATE =====" << std::endl;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] GameStateFlags::shouldRecordTime = "
            << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

        std::string playerName = Framework::PlayerNameManager::Get().GetName();
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] PlayerNameManager::GetName() = '" << playerName << "'" << std::endl;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] PlayerNameManager::HasName() = "
            << (Framework::PlayerNameManager::Get().HasName() ? "true" : "false") << std::endl;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] GameStateFlags::currentLevelName = '"
            << GameStateFlags::currentLevelName << "'" << std::endl;

        // Also get from TimerSystem
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] TimerSystem level name = '"
            << TimerSystem::Get().GetLevelName() << "'" << std::endl;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] ========================" << std::endl;

        // Stop the timer and submit to local leaderboard (if any)
        TimerSystem::Get().StopAndSubmit();

        // Check if we should record this time to Firebase
        bool shouldSubmit = GameStateFlags::shouldRecordTime && !playerName.empty();

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] Should submit to Firebase? " << (shouldSubmit ? "YES" : "NO") << std::endl;

        if (shouldSubmit) {
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "[TimerFinish] Submitting to Firebase with:" << std::endl;
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "  - Level: " << GameStateFlags::currentLevelName << std::endl;
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "  - Player: " << playerName << std::endl;
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "  - Time: " << finalTime << "s" << std::endl;

            FirebaseManager::Get().SubmitRun(
                GameStateFlags::currentLevelName,
                finalTime
            );
        }
        else {
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "[TimerFinish] NOT submitting to Firebase - conditions not met" << std::endl;
        }

        m_fired = true;
    }
}

void TimerFinishTriggerScript::OnDestroy() {
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[TimerFinish] Destroyed" << std::endl;
}