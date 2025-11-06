/*****************************************************************//**
* \file   AssetBrowser.h
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*      
*
* \brief    Contains declarations and definitions of an AssetBrowser that are
*           used to navigate, view, and manage assets within a specified directory.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ResourceManager.h"
#include <algorithm>
#include <imgui.h>

namespace fs = std::filesystem;
using Texture = Framework::Texture;
using ResourceManager = Framework::ResourceManager;

struct AssetEntry
{
	fs::path path;		   // Full path to the asset
    std::string name;      // File/folder name
    std::string display;   // What is shown in UI
    bool isDirectory = false;
    Texture iconTextureID = 0;
};

class AssetBrowser {
public:
    // Initializes the browser at the current working directory (executable's location)
    AssetBrowser(ResourceManager& rcm) : resourceManager_(&rcm), rootPath_(fs::current_path()), currentPath_(fs::current_path()) {
        if (fs::exists(currentPath_))
            ScanDirectory(currentPath_);
    }

	// Initializes the browser at a specified root path
    void Initialize(const fs::path& rootPath);

    // Core navigation functions
    void ScanDirectory(const fs::path& directoryPath);
	// Navigate to a specific path
    void NavigateTo(const fs::path& target);
	// Navigate up one level
    void NavigateUp();
	// Display folder tree recursively
    void ShowFolderTree(const fs::path& path);
    // UI rendering function (takes ImGui's current state/context)
    void RenderUI(std::vector<fs::path>& droppedFiles);
	// Renders the asset editor panel
    void RenderAssetEditor(std::vector<fs::path>& droppedFiles);

	// Handle files dropped into the asset browser
    void HandleDroppedFiles(const std::vector<fs::path>& droppedFiles);
	// Handle file replacement
    void HandleReplaceFile(const std::vector<fs::path>& droppedFiles);

	// Getters
    const std::vector<AssetEntry>& GetCurrentAssets() const { return currentAssets_; }
	// Get current path
    const fs::path& GetCurrentPath() const { return currentPath_; }

private:
    fs::path rootPath_;
    fs::path currentPath_;
	fs::path selectedAsset_;
    std::vector<AssetEntry> currentAssets_;
	ResourceManager* resourceManager_ = nullptr; // Pointer to ResourceManager for loading icons

	// Replacement mode variables
    bool isReplaceModeActive_ = false;
    fs::path replacementFilePath_;
    bool showReplaceConfirmation_ = false;

    fs::path localAssetsPath_;
    fs::path localDataPath_;
    fs::path repoAssetsPath_;
    fs::path repoDataPath_;
    bool syncToRepo_ = false;
};
