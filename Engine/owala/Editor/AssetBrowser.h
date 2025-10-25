#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ResourceManager.h"
#include <algorithm>

namespace fs = std::filesystem;
using namespace Framework;

struct AssetEntry
{
    fs::path path;
    std::string name;      // file/folder name
    std::string display;   // what is shown in UI
    bool isDirectory;
    Texture iconTextureID = 0;
};

class AssetBrowser {
public:
    // Initializes the browser at the current working directory (executable's location)
    AssetBrowser(ResourceManager& rcm) : resourceManager_(&rcm), rootPath_(fs::current_path()), currentPath_(fs::current_path()) {
        if (fs::exists(currentPath_))
            ScanDirectory(currentPath_);
    }

    void Initialize(const fs::path& rootPath);

    // Core navigation functions
    void ScanDirectory(const fs::path& directoryPath);
    void NavigateTo(const fs::path& target);
    void NavigateUp();
    void ShowFolderTree(const fs::path& path);
    // UI rendering function (takes ImGui's current state/context)
    void RenderUI(std::vector<fs::path>& droppedFiles);
    void RenderAssetEditor(std::vector<fs::path>& droppedFiles);

    void HandleDroppedFiles(const std::vector<fs::path>& droppedFiles);
    void HandleReplaceFile(const std::vector<fs::path>& droppedFiles);

    const std::vector<AssetEntry>& GetCurrentAssets() const { return currentAssets_; }
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
};
