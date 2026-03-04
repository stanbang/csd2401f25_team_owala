/*****************************************************************//**
 * \file	TestScript.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the test script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "TestScript.h"
#include "ScriptRegistry.h"

REGISTER_SCRIPT(TestScript, "TestScript")

void TestScript::OnCreate() {
	std::cout << "Entity with TestScript Created!" << std::endl;
}

void TestScript::OnUpdate() {
	std::cout << "Custom Script running" << std::endl;
}

void TestScript::OnDestroy() {

}