#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class SaveManager : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
private:
	int highestUnlockedLevel_ = 1;

	static constexpr const char* SAVE_FILEPATH = "Data/save.json";
	static constexpr const char* KEY_HIGHEST_UNLOCKED = "highest_unlocked_level";
public:
	bool IsUnlocked(int levelIndex) const;
	void Unlock(int levelIndex);

	int GetHighestProgression() const { return highestUnlockedLevel_; }
private:
	void Load();
	void Save() const;
};
