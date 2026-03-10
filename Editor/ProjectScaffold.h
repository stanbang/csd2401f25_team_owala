/*****************************************************************//**
* \file   ProjectScaffold.h
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief    This file contains the declarations of project scaffolding functions for the owala Engine Editor,
*			including creating a new project directory structure and writing the active game configuration file.
*			Used when creating a new project from the editor, and when switching active projects to update the CMake
*			configuration.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <string>

namespace Editor {
	/**
	 * Creates a new project directory structure with the specified root directory and project name.
	 * Generates the standard folders for assets, data, and source code, and populates them with default configuration files.
	 *
	 * \param rootDir
	 * \param projectName
	 * \return
	 */
	bool ScaffoldNewProject(const std::string& rootDir, const std::string& projectName);

	/**
	 * Checks if the given path is a valid project directory by verifying that it exists, is a directory,
	 * and contains at least one of the expected subfolders (e.g. "Source" or "Assets").
	 *
	 * \param path
	 * \return
	 */
	bool ValidateImportPath(const std::string& path);

	/**
	 * Writes the active_game.cmake file with the given game directory.
	 * This file is used by CMake to determine which game project to build
	 * and run when using the editor's Play button.
	 *
	 * \param gameDir
	 */
	void WriteActiveGameConfig(const std::string& gameDir);
}
