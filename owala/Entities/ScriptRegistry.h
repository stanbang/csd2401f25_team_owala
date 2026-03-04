/*****************************************************************//**
* \file   ScriptRegistry.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file defines the Runtime directory of native 
*			C++ script types.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "Components.h"

namespace Framework {
	class ScriptRegistry {
	public:
		struct RegistryEntry {
			void (*bind)(NativeScriptsComponent::NativeScript* script, const char* name);
			const char* name;
		};
	private:
		std::unordered_map<std::string, RegistryEntry> scriptMap;

		template<class T>
		static void BindScriptImpl(NativeScriptsComponent::NativeScript* script, const char* name) {
			script->Bind<T>(name);
		}

		template<class T>
		static ScriptableEntity* InstantiateImpl() {
			return new T();
		}

		static void DestroyImpl(NativeScriptsComponent::NativeScript* script) {
			delete script->Instance;
			script->Instance = nullptr;
		}

	public:
		static ScriptRegistry& Get() {
			static ScriptRegistry script_Registry;
			return script_Registry;
		}

		template <class T>
		void RegisterScript(const char* name) {
			RegistryEntry entry{};
			entry.bind = &BindScriptImpl<T>;
			entry.name = name;
			scriptMap[name] = entry;
			std::cout << "Registered Script" << name << std::endl;
		}

		bool BindScriptByName(NativeScriptsComponent::NativeScript& script, const std::string& name) const {
			std::unordered_map<std::string, RegistryEntry>::const_iterator found = scriptMap.find(name);
			if (found == scriptMap.end()) return false;
			found->second.bind(&script, found->second.name);
			return true;
		}

		template<typename F> void ForEachScriptName(F&& f) const {
			for (const std::pair<std::string, RegistryEntry>& element : scriptMap) f(element.first.c_str());
		}
	};
}

#define REGISTER_SCRIPT(TYPE, NAME_STR)                                      \
    namespace {                                                              \
        struct TYPE##_AutoReg {                                              \
            TYPE##_AutoReg() {                                               \
                ::Framework::ScriptRegistry::Get().RegisterScript<TYPE>(NAME_STR); \
            }                                                                \
        };                                                                   \
        static TYPE##_AutoReg TYPE##_AutoReg_Instance;                       \
    }