#pragma once
/*****************************************************************//**
 * \file	LevelSelection.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	2 February 2025
 *********************************************************************/
namespace LevelSelection {
	void Initialize();
	void Update();
	void Free();
	void ResetFade();

	// Fade state
    extern float fadeAlpha;
    extern bool fadingOut;
    extern std::string pendingScene;
}