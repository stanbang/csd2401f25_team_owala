#include "pch.h"
#include "TimerFinishTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"  // Add this include

REGISTER_SCRIPT(TimerFinishTriggerScript, "TimerFinishTriggerScript")

void TimerFinishTriggerScript::OnCreate() {
    m_fired = false;
    Framework::Logger::Get().Info(DebugType::PlayerSystem) << "[TimerFinish] Created" << std::endl;
}

void TimerFinishTriggerScript::OnUpdate() {
    if (m_fired) {
        // Already fired, ignore
        return;
    }

    if (!TimerSystem::Get().IsRunning()) {
        // Timer not running, ignore
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
        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "[TimerFinish] *** PORTAL REACHED! Stopping timer... ***" << std::endl;

        float finalTime = TimerSystem::Get().GetElapsed();
        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "[TimerFinish] Final time: " << finalTime << " seconds" << std::endl;

        TimerSystem::Get().StopAndSubmit();
        m_fired = true;
    }
}

void TimerFinishTriggerScript::OnDestroy() {}