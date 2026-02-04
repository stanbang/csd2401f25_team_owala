/*****************************************************************//**
 * \file	SaveManager.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the SaveManager script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

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
	bool skipIntro = false;

	static constexpr const char* SAVE_FILEPATH = "Data/save.json";
	static constexpr const char* KEY_HIGHEST_UNLOCKED = "highest_unlocked_level";
	static constexpr const char* KEY_SKIP_INTRO = "skip_intro";
public:
	bool IsUnlocked(int levelIndex) const;
	void Unlock(int levelIndex);

	void SetUnlock(int levelIndex);
	int GetHighestProgression() const { return highestUnlockedLevel_; }
	bool GetSkipIntro() const { return skipIntro; }
	void SetSkipIntro(bool value) { skipIntro = value; Save(); }
private:
	void Load();
	void Save() const;
};
