#pragma once
#include <string>

namespace Framework {
	class NativeScriptMaker {
	private:
		/* Internal Helpers */
		static std::string SanitizeClassName(const std::string& className);
		static std::string HeaderContent(const std::string& className);
		static std::string CppContent(const std::string& className, const std::string& header);
		static bool WriteFile(const std::string& path, const std::string& contents);
	public:
		static const bool CreateScriptPair(const std::string& className, const std::string& dirAbs, std::string& outMsg);
	};
}