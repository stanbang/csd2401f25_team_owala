#pragma once
#include "WindowsSystem.h"
extern GLFWwindow* g_window;
namespace Framework {
	// WASD
	bool IsKeyWPressed();
	bool IsKeyAPressed();
	bool IsKeySPressed();
	bool IsKeyDPressed();

	void UpdateMovement(float dt);
	float UpdatedMoveX();
	float UpdatedMoveY();
	void ResetMovement();
}