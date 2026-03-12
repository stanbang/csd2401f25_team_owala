#include "pch.h"
#include "TimerFinishTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(TimerFinishTriggerScript, "TimerFinishTriggerScript")

void TimerFinishTriggerScript::OnCreate() {
    m_fired = false;
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[TimerFinish] Created" << std::endl;
}

void TimerFinishTriggerScript::OnUpdate() {
    if (m_fired)                          return;
    if (!TimerSystem::Get().IsRunning())  return;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager  mgr(ecs);
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
            << "[TimerFinish] Portal reached! Elapsed: "
            << TimerSystem::Get().GetElapsed() << "s. Stopping timer." << std::endl;

        // Stop the timer. The OnRunFinished callback (set by game code) handles
        // Firebase submission, leaderboard display, etc.
        TimerSystem::Get().Stop();
        m_fired = true;
    }
}

void TimerFinishTriggerScript::OnDestroy() {
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[TimerFinish] Destroyed" << std::endl;
}