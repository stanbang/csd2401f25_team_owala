#include "NativeScriptMaker.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cctype>

namespace Framework {
	std::string NativeScriptMaker::SanitizeClassName(const std::string& className) {
		std::string sanitized_name;
		for (char c : className) {
			if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
				sanitized_name.push_back(c);
			}
		}

		if (sanitized_name.empty()) sanitized_name = "NewScript";
		if (std::isdigit(static_cast<unsigned char>(sanitized_name.front()))) {
			sanitized_name.insert(sanitized_name.begin(), '_');
		}
		
		return sanitized_name;
	}

	std::string NativeScriptMaker::HeaderContent(const std::string& className) {
		std::ostringstream oss;
		oss << "#pragma once\n\n";
		oss << "#include \"ScriptableEntity.h\"\n\n";
		oss << "using namespace Framework;\n\n";
		oss << "class " << className << " : public ScriptableEntity {\n";
		oss << "protected:\n";
		oss << "\tvoid OnCreate() override;\n";
		oss << "\tvoid OnUpdate() override;\n";
		oss << "\tvoid OnDestroy() override;\n";
		oss << "};\n";

		return oss.str();
	}

	std::string NativeScriptMaker::CppContent(const std::string& className, const std::string& header) {
		std::ostringstream oss;
		oss << "#include \"" + header + "\"\n";
		oss << "#include \"ScriptRegistry.h\"\n\n";
		oss << "REGISTER_SCRIPT(" + className + ", \"" + className + "\")\n\n";
		oss << "void " + className + "::OnCreate() {\n";
		oss << "\t// Initialization Code Here\n";
		oss << "}\n\n";
		oss << "void " + className + "::OnUpdate() {\n";
		oss << "\t// Per-Frame Logic Here\n";
		oss << "}\n\n";
		oss << "void " + className + "::OnDestroy() {\n";
		oss << "\t// Logic to be called before Entity is deleted\n";
		oss << "}\n";

		return oss.str();
	}

	bool NativeScriptMaker::WriteFile(const std::string& path, const std::string& contents) {
		try {
			std::ofstream outputFile(path, std::ios::binary);
			if (!outputFile) {
				Logger::Get().Error(DebugType::FileIOSystem) << "Could not open file: " << path << std::endl;
				return false;
			}

			outputFile << contents;
			if (!outputFile) {
				Logger::Get().Error(DebugType::FileIOSystem) << "Could not write file: " << path << std::endl;
				return false;
			}
			return true;
		}
		catch (const std::exception& e) {
			throw GameException(DebugType::FileIOSystem, e.what());
		}
	}

	const bool NativeScriptMaker::CreateScriptPair(const std::string& className, const std::string& dirAbs) {
		std::string sanitized_name = SanitizeClassName(className);
		if (sanitized_name.empty()) {
			Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: Invalid class name";
			return false;
		}

		std::error_code ec;
		std::filesystem::path dir(dirAbs);

		if (!std::filesystem::exists(dir)) {
			if (!std::filesystem::create_directories(dir, ec)) {
				Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: Failed to create directory: " << ec.message();
				return false;
			}
		} else if (!std::filesystem::is_directory(dir)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: Destination path is not a directory.";
			return false;
		}

		std::filesystem::path hpp = dir / (sanitized_name + ".h");
		std::filesystem::path cpp = dir / (sanitized_name + ".cpp");

		if (std::filesystem::exists(hpp) || std::filesystem::exists(cpp)) {
			Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: File(s) already exist: " + hpp.string() + " / " + cpp.string();
			return false;
		}

		// Write Header
		std::ofstream headerFile(hpp);
		if (!headerFile) {
			Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: Failed to create header file: " + hpp.string();
			return false;
		}
		headerFile << HeaderContent(sanitized_name);
		headerFile.close();

		// Write Source
		std::ofstream sourceFile(cpp);
		if (!sourceFile) {
			std::error_code remEc;
			std::filesystem::remove(hpp, remEc);
			Logger::Get().Info(DebugType::FileIOSystem) << "CreateScriptPair: Failed to create source file: " + cpp.string();
			return false;
		}
		std::string headerInclude = sanitized_name;
		headerInclude += ".h";
		sourceFile << CppContent(sanitized_name, headerInclude);
		sourceFile.close();

		return true;
	}
}