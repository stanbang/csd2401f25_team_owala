/*****************************************************************//**
* \file   NativeScriptMaker.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file declares the Utility interface NativeScriptMaker used
*			for generating new native C++ script files.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
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
		/* Interface APIs */
		// Creates a Native Script Header and Source file with the given class name at the target directory.
		static const bool CreateScriptPair(const std::string& className, const std::string& dirAbs, std::string& outMsg);
	};
}