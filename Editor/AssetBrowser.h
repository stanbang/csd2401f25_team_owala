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
    // Starts the browser from the current working directory.
    // Useful as a safe default before a project is explicitly loaded.
    AssetBrowser(ResourceManager& rcm) : resourceManager_(&rcm), rootPath_(fs::current_path()), currentPath_(fs::current_path()) {
        if (fs::exists(currentPath_))
            ScanDirectory(currentPath_);
    }

	// Sets the project root and prepares local/repo sync paths.
    void Initialize(const fs::path& rootPath);

    // Rebuilds the current asset list from a directory on disk.
    void ScanDirectory(const fs::path& directoryPath);
	// Enters a folder or selects a file.
    void NavigateTo(const fs::path& target);
	// Moves up one folder (clamped to rootPath_).
    void NavigateUp();
	// Draws the left-hand folder tree recursively.
    void ShowFolderTree(const fs::path& path);
    // Draws Project Explorer + Asset Viewer windows.
    void RenderUI(std::vector<fs::path>& droppedFiles);
	// Draws the right-hand Asset Editor panel.
    void RenderAssetEditor(std::vector<fs::path>& droppedFiles);

	// Queues files dropped from OS/file explorer into the viewer.
    void HandleDroppedFiles(const std::vector<fs::path>& droppedFiles);
	// Handles replacement mode drops (same extension guard).
    void HandleReplaceFile(const std::vector<fs::path>& droppedFiles);

	// Getters
    const std::vector<AssetEntry>& GetCurrentAssets() const { return currentAssets_; }
	// Get current path
    const fs::path& GetCurrentPath() const { return currentPath_; }

private:
    // Loads and caches viewer icon textures once per session.
    void EnsureIconTexturesLoaded();
    // Initializes a batch upload state for dropped files.
    void StartUploadQueue(const std::vector<fs::path>& droppedFiles);
    // Processes one queued upload item per frame.
    void ProcessUploadQueue();
    // Copies one dropped file to local output and optional repo mirror.
    bool CopyDroppedFile(const fs::path& sourcePath);
    // Shortens a label to fit the tile width, using "..." when needed.
    std::string TruncateLabelToFit(const std::string& text, float maxWidth, bool& wasTruncated) const;

    // Browser navigation state
    fs::path rootPath_;
    fs::path currentPath_;
    fs::path projectRootPath_;
	fs::path selectedAsset_;
    std::vector<AssetEntry> currentAssets_;
	ResourceManager* resourceManager_ = nullptr;

	// Replace/delete UI state
    bool isReplaceModeActive_ = false;
    fs::path replacementFilePath_;
    bool showReplaceConfirmation_ = false;
    bool showDeleteConfirmation_ = false;

    // Local output paths (build/Debug) and source project mirror paths
    fs::path localAssetsPath_;
    fs::path localDataPath_;
    fs::path repoAssetsPath_;
    fs::path repoDataPath_;
    bool syncToRepo_ = false;

    // Cached icon textures used by non-image asset tiles
    bool iconsLoaded_ = false;
    Texture folderIconTexture_ = 0;
    Texture fileIconTexture_ = 0;
    Texture fontIconTexture_ = 0;
    Texture audioIconTexture_ = 0;

    // Upload overlay / popup state
    std::vector<fs::path> uploadQueue_;
    bool uploadInProgress_ = false;
    bool showUploadPopup_ = false;
    bool uploadHadErrors_ = false;
    size_t uploadTotalFiles_ = 0;
    size_t uploadProcessedFiles_ = 0;
    double uploadPopupDismissAt_ = 0.0;
    std::string uploadCurrentFile_;

    // Drag payload path used by ImGui drag-drop source.
    std::string currentDraggedPath_;
};
