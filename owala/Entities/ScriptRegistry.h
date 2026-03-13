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
		// Stores the metadata required to bind a script
		struct RegistryEntry {
			void (*bind)(NativeScriptsComponent::NativeScript* script, const char* name);
			const char* name;
		};
	private:
		// Maps script string names to their registry entries
		std::unordered_map<std::string, RegistryEntry> scriptMap;

		// Internal helper to bind a specific script of type T to a NativeScript component
		template<class T>
		static void BindScriptImpl(NativeScriptsComponent::NativeScript* script, const char* name) {
			script->Bind<T>(name);
		}

		// Internal helper to allocate a new script instance
		template<class T>
		static ScriptableEntity* InstantiateImpl() {
			return new T();
		}

		// Internal helper to deallocate a script instance
		static void DestroyImpl(NativeScriptsComponent::NativeScript* script) {
			delete script->Instance;
			script->Instance = nullptr;
		}

	public:
		// Returns the singleton instance of the ScriptRegistry as reference
		static ScriptRegistry& Get() {
			static ScriptRegistry script_Registry;
			return script_Registry;
		}

		// Registers a new script type into the registry
		template <class T>
		void RegisterScript(const char* name) {
			RegistryEntry entry{};
			entry.bind = &BindScriptImpl<T>;
			entry.name = name;
			scriptMap[name] = entry;
			std::cout << "Registered Script" << name << std::endl;
		}

		// Binds a script to a component using it's registered string name. Returns true if the script was found. Otherwise returns false if the script is not in the registry.
		bool BindScriptByName(NativeScriptsComponent::NativeScript& script, const std::string& name) const {
			std::unordered_map<std::string, RegistryEntry>::const_iterator found = scriptMap.find(name);
			if (found == scriptMap.end()) return false;
			found->second.bind(&script, found->second.name);
			return true;
		}

		// Iterates over all registered script names and executes a callback
		template<typename F> void ForEachScriptName(F&& f) const {
			for (const std::pair<std::string, RegistryEntry>& element : scriptMap) f(element.first.c_str());
		}
	};
}

/**
 * @brief Macro for automatic script registration during runtime.
 * * Creates an anonymous namespace with a static struct. The struct's constructor 
 * automatically calls RegisterScript on the singleton instance, ensuring the 
 * script is available at runtime without manual registration calls.
 * * @param TYPE The C++ class name of the script.
 * @param NAME_STR The string literal representing the script's name.
 */
#define REGISTER_SCRIPT(TYPE, NAME_STR)                                      \
    namespace {                                                              \
        struct TYPE##_AutoReg {                                              \
            TYPE##_AutoReg() {                                               \
                ::Framework::ScriptRegistry::Get().RegisterScript<TYPE>(NAME_STR); \
            }                                                                \
        };                                                                   \
        static TYPE##_AutoReg TYPE##_AutoReg_Instance;                       \
    }