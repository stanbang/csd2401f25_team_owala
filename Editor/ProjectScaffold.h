/*********************************************************************//**
* \file   ProjectScaffold.h
*
* \brief  Declares free utility functions for creating and importing
*         game projects in the Owala Engine editor.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <string>

namespace Editor {
	// Creates the standard game-project folder scaffold under rootDir/projectName/.
	// Returns true on success.
	bool ScaffoldNewProject(const std::string& rootDir, const std::string& projectName);

	// Validates that the given path looks like a valid (pre-existing) game project folder.
	// Returns true when the path exists and contains at least a "Source" or "Assets" subfolder.
	bool ValidateImportPath(const std::string& path);

	// Writes active_game.cmake at the repository root to point the build system at gameDir.
	void WriteActiveGameConfig(const std::string& gameDir);
}
