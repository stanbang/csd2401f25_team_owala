/*****************************************************************//**
* \file   BuildSizeAnalyzer.h
*
* \primary_author
*       Yar Xuan
*
* \brief    Header file for the BuildSizeAnalyzer in the Editor module.
*			The BuildSizeAnalyzer is an editor tool that scans project directories, displays asset sizes,
*			and allows the user to export selected assets to a destination folder.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <cstdint>
namespace Editor {

	struct BuildAssetItem {
		std::string relativePath; // Path relative to the source directory
		std::string filename;     // Name of the file
		uintmax_t size = 0;       // File size in bytes
		bool selected = true;     // Whether this asset is checked for export
	};

	class BuildSizeAnalyzer {
	public:
		BuildSizeAnalyzer() = default;
		~BuildSizeAnalyzer() = default;

		// Renders the ImGui window and all its internal UI components.
		// Should be called every frame in the Editor's update loop.
		void ShowUI();

		// --- Getters and Setters for Window State ---
		bool IsOpen() const { return showBuildSizeAnalyzer_; }
		void SetOpen(bool open) { showBuildSizeAnalyzer_ = open; }
		void ToggleOpen() { showBuildSizeAnalyzer_ = !showBuildSizeAnalyzer_; }

	private:
		bool showBuildSizeAnalyzer_ = false;
		bool buildAnalyzerInitialized_ = false;

		std::vector<BuildAssetItem> buildAssets_;  // List of all discovered assets
		uintmax_t totalSelectedBuildSize_ = 0;     // Cached total size of all selected assets (in bytes)
		char exportDestinationBuffer_[512] = "";   // ImGui text buffer for the export destination path

		// --- Internal Helpers ---

		// Scans the source directory for files and updates the buildAssets_ list.
		void RefreshBuildAssets();

		// Recalculates the totalSelectedBuildSize_ by summing the sizes of checked items.
		void CalculateTotalSelectedBuildSize();

#ifdef _WIN32
		// Native Windows helper to open the standard folder picker dialog.
		std::string OpenFolderBrowser();
#endif

		// Copies all selected assets to the destination folder specified in exportDestinationBuffer_.
		void ExportSelectedBuildAssets();

		// Converts raw bytes into a human-readable string (e.g., "1.50 MB").
		std::string FormatBytes(uintmax_t bytes) const;
	};

}