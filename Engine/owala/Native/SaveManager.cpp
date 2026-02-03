#include "SaveManager.h"
#include "ScriptRegistry.h"

// Dependencies
#include "FileUtility.h"
#include "SceneManager.h"
#include "GameManager.h"


REGISTER_SCRIPT(SaveManager, "SaveManager")

bool SaveManager::IsUnlocked(int levelIndex) const {
	return levelIndex <= highestUnlockedLevel_;
}

void SaveManager::Unlock(int levelIndex) {
	if (levelIndex > highestUnlockedLevel_) {
		highestUnlockedLevel_ = levelIndex;
		Save();
	}
}

void SaveManager::Load() {
	rapidjson::Document doc;

	// Check if file exists
	if (!Framework::FileIOSystem::ReadFile(SAVE_FILEPATH, doc) || !doc.IsObject()) {
		highestUnlockedLevel_ = 1;
		Save();
		return;
	}

	if (doc.HasMember(KEY_HIGHEST_UNLOCKED) && doc[KEY_HIGHEST_UNLOCKED].IsInt()) {
		highestUnlockedLevel_ = doc[KEY_HIGHEST_UNLOCKED].GetInt();
		if (highestUnlockedLevel_ < 1) highestUnlockedLevel_ = 1;
	}
	else {
		highestUnlockedLevel_ = 1;
	}
}

void SaveManager::Save() const {
	rapidjson::Document doc;

	doc.SetObject();
	rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

	doc.AddMember(rapidjson::StringRef(KEY_HIGHEST_UNLOCKED), highestUnlockedLevel_, alloc);

	Framework::FileIOSystem::WriteFileJson(SAVE_FILEPATH, doc);
}

void SaveManager::OnCreate() {
	Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
	
	// In case this isn't persistent.
	sceneMgr.SetEntityAsPersistent(GetEntity(), true);
	Load();
}

void SaveManager::OnUpdate() {
	// Per-Frame Logic Here
}

void SaveManager::OnDestroy() {
	// Logic to be called before Entity is deleted
}