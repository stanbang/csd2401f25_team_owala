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

	// Fade state
	static float fadeAlpha = 0.0f;
	static bool  fadingOut = false;
	static std::string pendingScene = "";
}