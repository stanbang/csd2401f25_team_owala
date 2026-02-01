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