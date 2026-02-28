/**********************************************************************
 * \file	TestScene.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	20 September 2025
 *********************************************************************/
#include "TestScene.h"

#include "GameManager.h"
#include "PhysicsSystem.h"
#include "PrefabSystem.h"

namespace TestScene {
	using namespace Framework;

	// Track which scenes have already been initialized by their GUID.
	// A GUID is inserted on first Initialize and erased only when that
	// exact scene is fully unloaded (i.e. its own fpFree fires while
	// it is still the active/known scene — captured at Initialize time).
	static std::unordered_set<SceneGUID> initializedSceneGUIDs;

	void Initialize() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		CameraSystem* cameraSystem = gm.GetSystems().camera_;
		(void)audio;

		// Capture the GUID of the scene being initialized right now.
		const SceneGUID currentSceneGUID = sceneMgr.GetActiveScene().sceneIndex;
		const bool isFirstInitialization = !initializedSceneGUIDs.count(currentSceneGUID);

		gm.GetSystems().gfx_->setAlphaBlend(true);

		EntityManager entityMgr(*ecs);

		std::vector<Entity> players = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
		Entity playerEntity = 0;
		if (!players.empty()) {
			playerEntity = players[0];
		}
		else {
			Framework::Logger::Get().Error(DebugType::GameManager)
				<< "[TestScene] Player missing! PlayerCount: "
				<< players.size() << std::endl;
		}

		std::vector<Entity> cameras = entityMgr.GetAllEntitiesWithComponent(true, CameraType);
		Entity mainCamera = 0;
		if (!cameras.empty()) {
			mainCamera = cameras[0];
			cameraSystem->SetActiveCamera(mainCamera);
			sceneMgr.SetEntityAsPersistent(mainCamera);

			Camera& gameCam = cameraSystem->GetCamera(mainCamera);
			gameCam.isEditorControlled = false;
		}

		if (playerEntity && mainCamera) {
			cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);
			cameraSystem->CalculateAndSetBoundsFromPlatforms(mainCamera, 0.0f);
		}

		// Spawn player at SpawnPoint only on the very first initialization of this scene.
		if (isFirstInitialization) {
			initializedSceneGUIDs.insert(currentSceneGUID);

			std::vector<Entity> spawnPts = entityMgr.GetAllEntitiesWithComponents(true, SpawnPointTagType, LoadedTagType, TransformType);
			if (!spawnPts.empty()) {
				if (entityMgr.HasComponent(playerEntity, TransformType)) {
					Entity playerSpawnPt = spawnPts[0];
					Transform& playerTransform = entityMgr.GetComponent<Transform>(playerEntity);
					Transform spawnPtTransform = entityMgr.GetComponent<Transform>(playerSpawnPt);

					playerTransform.position = spawnPtTransform.position;
					Framework::Logger::Get().Info(DebugType::GameManager)
						<< "[TestScene] Spawned Player at ("
						<< playerTransform.position.x << ", "
						<< playerTransform.position.y << ")" << std::endl;
				}
				else {
					Framework::Logger::Get().Error(DebugType::GameManager)
						<< "[TestScene] Player does not have Transform component!" << std::endl;
				}
			}
			else {
				Framework::Logger::Get().Error(DebugType::GameManager)
					<< "[TestScene] Player SpawnPoint missing! SpawnPtCount: "
					<< spawnPts.size() << std::endl;
			}
		}

		// If this scene was already initialized before, it means we're returning to it from another scene (e.g. PauseScene or OptionsMenu). In that case, do NOT respawn the player or reset the camera because we want them to be exactly as they were when we left. Just return early from Initialize and let Update take over.
		const SceneAsset& activeScene = sceneMgr.GetActiveScene();
		if (!gm.IsLevelScene(activeScene.name)) {
			return;
		}
	}

	void Update() {
	}

	void Free() {
		// Intentionally empty: do NOT read GetActiveScene() here.
		// Free() is called by GameManager when any scene that shares this
		// callback (including PauseScene / OptionsMenu) is unloaded, so
		// GetActiveScene() would return the wrong GUID and would incorrectly
		// erase the level scene from initializedSceneGUIDs.
		//
		// Level GUIDs are naturally invalidated when a full scene reload
		// occurs because LoadScene assigns a fresh GUID that has never been
		// inserted into the set.
	}
}