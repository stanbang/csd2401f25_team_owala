/*****************************************************************//**
* \file   AssetBrowser.cpp
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief    Contains the implementation of an AssetBrowser that allows the user
*           to navigate, view, and manage assets within a specified directory.
*           Comes with a folder tree view, asset viewer, and asset editor panel
*           that support drag-and-drop file importing and asset replacement.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "AssetBrowser.h"
#include "Logger.h"

using namespace Framework;

// Initialize the AssetBrowser with a specified root path
void AssetBrowser::Initialize(const fs::path& rootPath)
{
    // Validate input path
    if (rootPath.empty()) {
        Logger::Get().Error(DebugType::Editor) << "AssetBrowser: Empty root path provided!" << std::endl;
        return;
    }

	// If input path is valid, get the absolute path
    rootPath_ = fs::absolute(rootPath);
    currentPath_ = rootPath_;

    localAssetsPath_ = rootPath_ / "Assets";
    localDataPath_ = rootPath_ / "Data";

    // Get the path to the project root (two levels up from Debug)
    fs::path projectRoot = fs::absolute(rootPath_ / "../../");
    repoAssetsPath_ = projectRoot / "Assets";
    repoDataPath_ = projectRoot / "Data";

    // Check if all 4 directories exist to enable sync
    bool localAssetsExists = fs::exists(localAssetsPath_) && fs::is_directory(localAssetsPath_);
    bool localDataExists = fs::exists(localDataPath_) && fs::is_directory(localDataPath_);
    bool repoAssetsExists = fs::exists(repoAssetsPath_) && fs::is_directory(repoAssetsPath_);
    bool repoDataExists = fs::exists(repoDataPath_) && fs::is_directory(repoDataPath_);

    if (localAssetsExists && localDataExists && repoAssetsExists && repoDataExists)
    {
        syncToRepo_ = true;
        Logger::Get().Info(DebugType::Editor) << "Repo sync enabled." << std::endl;
        Logger::Get().Info(DebugType::Editor) << "  Local Assets: " << localAssetsPath_.string() << std::endl;
        Logger::Get().Info(DebugType::Editor) << "  Local Data: " << localDataPath_.string() << std::endl;
        Logger::Get().Info(DebugType::Editor) << "  Repo Assets: " << repoAssetsPath_.string() << std::endl;
        Logger::Get().Info(DebugType::Editor) << "  Repo Data: " << repoDataPath_.string() << std::endl;
    }
    else
    {
        syncToRepo_ = false;
        Logger::Get().Info(DebugType::Editor) << "Repo sync disabled. Missing one or more required directories." << std::endl;
        if (!localAssetsExists) Logger::Get().Error(DebugType::Editor) << "Could not find local 'Assets' folder at: " << localAssetsPath_.string();
        if (!localDataExists) Logger::Get().Error(DebugType::Editor) << "Could not find local 'Data' folder at: " << localDataPath_.string();
        if (!repoAssetsExists) Logger::Get().Error(DebugType::Editor) << "Could not find repo 'Assets' folder at: " << repoAssetsPath_.string();
        if (!repoDataExists) Logger::Get().Error(DebugType::Editor) << "Could not find repo 'Data' folder at: " << repoDataPath_.string();
    }

    Logger::Get().Info(DebugType::Editor) << "AssetBrowser initializing with root: '" << rootPath_.string() << "'" << std::endl;

	// Check if the root path exists and is a directory
    if (fs::exists(rootPath_) && fs::is_directory(rootPath_))
        ScanDirectory(rootPath_);
    else
        Logger::Get().Error(DebugType::Editor) << "AssetBrowser: Root path invalid: " << rootPath_.string() << std::endl;
}

// Scan a directory and populate the asset list
void AssetBrowser::ScanDirectory(const fs::path& directoryPath)
{
	// Log incoming directory path
    Logger::Get().Info(DebugType::Editor) << "ScanDirectory called with: '" << directoryPath.string() << "'" << std::endl;

    // Validate the path before scanning
	// Check for empty path
    if (directoryPath.empty()) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Empty path provided!" << std::endl;
        return;
    }

	// If path does not exist or is not a directory, log error and return
    if (!fs::exists(directoryPath)) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Path does not exist: '" << directoryPath.string() << "'" << std::endl;
        return;
    }

	// If path is not a directory, log error and return
    if (!fs::is_directory(directoryPath)) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Path is not a directory: '" << directoryPath.string() << "'" << std::endl;
        return;
    }

	// Clear current assets in the list
    currentAssets_.clear();
    currentPath_ = directoryPath;
    Logger::Get().Info(DebugType::Editor) << "Assigned currentPath_: '" << currentPath_.string() << "'" << std::endl;

    // Parent folder entry, if not at root, generates a ".." button to go up one level in the Asset Viewer
    if (currentPath_ != rootPath_)
    {
        fs::path parentPath = fs::absolute(currentPath_.parent_path());
        Logger::Get().Info(DebugType::Editor) << "Adding parent entry: '" << parentPath.string() << "'" << std::endl;

        currentAssets_.push_back({
            .path = parentPath,
            .name = "..",
            .display = "..",
            .isDirectory = true
            });
    }

	// After generation of parent entry(or not if at root already), scan the directory for files and folders and create AssetEntry for each
    try
    {
        int fileCount = 0;
        int dirCount = 0;

		// Iterate through directory entries
        for (const fs::directory_entry& entry : fs::directory_iterator(currentPath_))
        {
			// Create an AssetEntry for each file/folder and store it in currentAssets_
            AssetEntry asset;
            asset.path = fs::absolute(entry.path());
            asset.name = entry.path().filename().string();
            asset.display = asset.name;
            asset.isDirectory = entry.is_directory();

            if (asset.isDirectory) {
                dirCount++;
            }
            else {
                fileCount++;
            }

            currentAssets_.push_back(std::move(asset));
        }

        Logger::Get().Info(DebugType::Editor) << "Found " << dirCount << " directories and " << fileCount << " files" << std::endl;
    }
    catch (const fs::filesystem_error& e)
    {
        Logger::Get().Error(DebugType::Editor) << "Failed to scan directory: " << e.what() << std::endl;
        // fallback to parent
        if (currentPath_.has_parent_path() && currentPath_ != rootPath_)
            currentPath_ = currentPath_.parent_path();
    }

    // Sort directories first, then files alphabetically
    std::sort(currentAssets_.begin(), currentAssets_.end(), [](const AssetEntry& a, const AssetEntry& b) {
        if (a.isDirectory != b.isDirectory)
            return a.isDirectory > b.isDirectory;
        return a.name < b.name;
        });

    Logger::Get().Info(DebugType::Editor) << "Total assets after sort: " << currentAssets_.size() << std::endl;
}

// Navigate to a specific path
void AssetBrowser::NavigateTo(const fs::path& target)
{
    Logger::Get().Info(DebugType::Editor) << "NavigateTo called with: '" << target.string() << "'" << std::endl;

	// Validate the target path
	// Check for empty path
    if (target.empty()) {
        Logger::Get().Error(DebugType::Editor) << "NavigateTo: Empty path provided!" << std::endl;
        return;
    }

	// Check if path exists
    if (!fs::exists(target)) {
        Logger::Get().Error(DebugType::Editor) << "NavigateTo: Path does not exist: '" << target.string() << "'" << std::endl;
        return;
    }

	// If target is a directory, scan it to prepare the Asset Viewer for display
    if (fs::is_directory(target)) {
        // Make a copy to avoid reference issues
        fs::path targetCopy = target;
        Logger::Get().Info(DebugType::Editor) << "Navigating to directory: '" << targetCopy.string() << "'" << std::endl;
        ScanDirectory(targetCopy);
    }
    else {
        Logger::Get().Info(DebugType::Editor) << "File selected: " << target.string() << std::endl;
    }
}

// Navigate up one level
void AssetBrowser::NavigateUp()
{
    Logger::Get().Info(DebugType::Editor) << "NavigateUp called. Current: '" << currentPath_.string() << "', Root: '" << rootPath_.string() << "'" << std::endl;

	// If already at root, don't let the user go up further
    if (currentPath_ == rootPath_) {
        Logger::Get().Info(DebugType::Editor) << "Already at root, cannot navigate up" << std::endl;
        return;
    }

    fs::path parent = currentPath_.parent_path();
    Logger::Get().Info(DebugType::Editor) << "Parent path: '" << parent.string() << "'" << std::endl;

    // Check if parent is within root
    std::string parentStr = parent.string();
    std::string rootStr = rootPath_.string();

	// If parent is outside root, navigate to root instead
    if (parentStr.find(rootStr) != 0) {
        Logger::Get().Info(DebugType::Editor) << "Parent outside root, navigating to root instead" << std::endl;
        parent = rootPath_;
    }

	// Scan the parent directory to update the Asset Viewer
    ScanDirectory(parent);
}

// Display folder tree recursively
void AssetBrowser::ShowFolderTree(const fs::path& path)
{
	// Validate the path
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }

    try {
        for (const fs::directory_entry& entry : fs::directory_iterator(path))
        {
            const std::string name = entry.path().filename().string(); // Get the name of the file/folder
			bool isDir = entry.is_directory(); // Get whether it's a directory or not
			ImGuiTreeNodeFlags flags = isDir ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf; // Set openable flags based on type

			// Render the tree node
            bool opened = ImGui::TreeNodeEx(name.c_str(), flags);

            // Log clicks for debug
            if (ImGui::IsItemClicked() && !isDir)
            {
                Logger::Get().Info(DebugType::Editor) << "File clicked: " << entry.path().string() << std::endl;
            }
			// Handle double-clicks on directories to navigate into them
            else if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0) && isDir)
            {
                Logger::Get().Info(DebugType::Editor) << "Folder double-clicked: " << entry.path().string() << std::endl;
                NavigateTo(entry.path());
            }

			// If it's a directory and opened, recurse into it to show its contents
            if (opened && isDir)
            {
                ShowFolderTree(entry.path());
                ImGui::TreePop();
            }
			// Else, if it's not a directory but opened (leaf node), just pop the tree
            else if (opened)
            {
                ImGui::TreePop();
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        Logger::Get().Error(DebugType::Editor) << "Error in ShowFolderTree: " << e.what() << std::endl;
    }
}

// Render the Asset Browser UI
void AssetBrowser::RenderUI(std::vector<fs::path>& droppedFiles)
{
    // Folder Tree Panel
    if (ImGui::Begin("Project Explorer"))
    {
        if (ImGui::Button("Go to Root"))
            NavigateTo(rootPath_);

        ImGui::Separator();
        ShowFolderTree(rootPath_);
    }
    ImGui::End();

    // Asset Viewer Panel
    if (ImGui::Begin("Asset Viewer"))
    {
		ImGui::TextWrapped("Directory: %s", currentPath_.string().c_str()); // List the current directory path for easy reference
        ImGui::Text("Assets: %zu", currentAssets_.size());
        ImGui::Separator();

		ImGui::BeginChild("AssetList", ImVec2(0, 0), true); // The actual area that lists the assets in a grid
        
		// Handle file drops onto the Asset Viewer area from the OS's file explorer
        if (!droppedFiles.empty() && !isReplaceModeActive_ &&
            (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow) ||
                ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow)))
        {
            HandleDroppedFiles(droppedFiles);
            droppedFiles.clear();
        }

		// Calculate number of columns based on available width
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = 120.0f;
        int columns = std::max(1, (int)(availableWidth / itemWidth));
        ImGui::Columns(columns, "assetColumns", false);

		// Loop through assets and render them
        for (size_t i = 0; i < currentAssets_.size(); ++i)
        {
            const auto& asset = currentAssets_[i];
            ImGui::PushID(static_cast<int>(i));

            ImVec2 itemSize(100, 100);

			// Render directory entries as buttons
            if (asset.isDirectory)
            {
                std::string displayName = "[DIR] " + asset.name;
                if (ImGui::Button(displayName.c_str(), itemSize))
                {
					if (asset.name == "..") // Go up one level button
                        NavigateUp();
                    else
						NavigateTo(asset.path); // Navigate into the clicked directory
                }
            }
			// Render file entries
            else
            {
				// Determine file extension
                std::string ext = asset.path.extension().string();
				// Convert extension to lowercase for comparison
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); }); 

                bool clicked = false;

                // Supported image extensions 
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                {
                    std::string texName = asset.path.filename().string();

                    // Check if the resource exists for display
                    if (!resourceManager_->has<Texture>(texName))
                    {
                        // If not, try to load it
                        resourceManager_->loadTexture(texName, asset.path.string());
                    }

					// Get and display the texture after loading from resource manager
                    try
                    {
                        Texture& tex = resourceManager_->get<Texture>(texName);
                        clicked = ImGui::ImageButton(("asset_" + asset.name).c_str(), tex, itemSize, ImVec2(0, 1), ImVec2(1, 0));
                    }
					// Catch any exceptions during texture retrieval
                    catch (const std::exception& e)
                    {
                        Logger::Get().Info(DebugType::Editor) << "Failed to load file " << asset.name << ": " << e.what() << std::endl;
                    }

					// Handle drag-and-drop for image files
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                    {
                        const char* payload_type = "ASSET_PATH";
                        currentDraggedPath_ = asset.path.string();
                        ImGui::SetDragDropPayload(payload_type, currentDraggedPath_.c_str(),
                            (currentDraggedPath_.size() + 1) * sizeof(char));
                        ImGui::Text("Dragging: %s", asset.name.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

				// If file image button was clicked, select the asset
                if (clicked)
                {
                    selectedAsset_ = asset.path;
                    Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
                }

				// Non-image files are rendered as regular buttons
                else if (!(ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                {
                    if (ImGui::Button(asset.display.c_str(), itemSize))
                    {
                        selectedAsset_ = asset.path;
                        Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
                    }

					// Handle drag-and-drop for non-image files
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                    {
                        const char* payload_type = "ASSET_PATH";
                        currentDraggedPath_ = asset.path.string();
                        ImGui::SetDragDropPayload(payload_type, currentDraggedPath_.c_str(),
                            (currentDraggedPath_.size() + 1) * sizeof(char));
                        ImGui::Text("Dragging: %s", asset.name.c_str());
                        ImGui::EndDragDropSource();
                    }
                }


            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::EndChild();
    }
    ImGui::End();

	// Render Asset Editor Panel after updating backend
    RenderAssetEditor(droppedFiles);
}

// Render the Asset Editor panel
void AssetBrowser::RenderAssetEditor(std::vector<fs::path>& droppedFiles)
{
    if (ImGui::Begin("Asset Editor")) {
        // If selected asset is not empty, aka something is selected
        if (!selectedAsset_.empty()) {
            ImGui::TextWrapped("Selected Asset: %s", selectedAsset_.string().c_str());
            ImGui::Separator();
            std::string ext = selectedAsset_.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); }); 

			// Reimport Button(Reloads the asset from disk)
            if (ImGui::Button("Reimport Asset", ImVec2(150, 30)))
            {
                Logger::Get().Info(DebugType::Editor) << "Reimporting asset: " << selectedAsset_.filename().string() << std::endl;

                try
                {
                    // Unload old texture if needed
                    resourceManager_->unload<Texture>(selectedAsset_.filename().string());

                    // Reload texture
                    if (resourceManager_->loadTexture(selectedAsset_.filename().string(), selectedAsset_.string()))
                    {
                        Logger::Get().Info(DebugType::Editor) << "Reimport successful for: " << selectedAsset_.filename().string() << std::endl;
                    }
                    else
                    {
                        Logger::Get().Error(DebugType::Editor) << "Reimport failed for: " << selectedAsset_.filename().string() << std::endl;
                    }
                }
                catch (const std::exception& e)
                {
                    Logger::Get().Error(DebugType::Editor) << "Reimport exception: " << e.what() << std::endl;
                }
            }

            // Replace Button (Toggles replace mode that allows drag and drop into a drop zone)
            ImGui::SameLine();
            if (ImGui::Button("Replace Asset", ImVec2(150, 30)))
            {
                isReplaceModeActive_ = !isReplaceModeActive_;
                if (!isReplaceModeActive_)
                {
                    // Cancel replace mode
                    replacementFilePath_.clear();
                    showReplaceConfirmation_ = false;
                }
            }

			// Delete Button
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Red button for delete
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Delete Asset", ImVec2(150, 30)))
            {
                showDeleteConfirmation_ = true;
            }
            ImGui::PopStyleColor(3);

            // Drop zone for replacement
            if (isReplaceModeActive_)
            {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Replace Mode Active");
                ImGui::Text("Drag and drop a replacement file below:");

                // Create a drop zone
                ImVec2 dropZoneSize(400, 100);

                ImGui::BeginChild("ReplaceDropZone", dropZoneSize, true, ImGuiWindowFlags_NoScrollbar);

                // Handle drops on the drop zone
                if (!droppedFiles.empty() && ImGui::IsWindowHovered())
                {
                    HandleReplaceFile(droppedFiles);
                    droppedFiles.clear();
                }

                // Visual feedback for drop zone
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 min = ImGui::GetWindowPos();
                ImVec2 max = ImVec2(min.x + dropZoneSize.x, min.y + dropZoneSize.y);

                // Dashed border effect
                ImU32 borderColor = IM_COL32(100, 150, 255, 255);
                if (ImGui::IsWindowHovered())
                {
                    borderColor = IM_COL32(150, 200, 255, 255);
                    drawList->AddRectFilled(min, max, IM_COL32(50, 100, 150, 50));
                }
                drawList->AddRect(min, max, borderColor, 0.0f, 0, 2.0f);

                // Center text in drop zone
                const char* dropText = "Drop file here";
                ImVec2 textSize = ImGui::CalcTextSize(dropText);
                ImGui::SetCursorPos(ImVec2(
                    (dropZoneSize.x - textSize.x) * 0.5f,
                    (dropZoneSize.y - textSize.y) * 0.5f
                ));
                ImGui::TextDisabled("%s", dropText);

                ImGui::EndChild();

                if (ImGui::Button("Cancel Replace", ImVec2(150, 25)))
                {
                    isReplaceModeActive_ = false;
                    replacementFilePath_.clear();
                    showReplaceConfirmation_ = false;
                }
            }

            // Replace Confirmation Popup
            if (showReplaceConfirmation_)
            {
                ImGui::OpenPopup("Confirm Replace");
            }

			// Delete Confirmation Popup
            if (showDeleteConfirmation_)
            {
                ImGui::OpenPopup("Confirm Delete");
            }

            // Replace Confirmation Modal
            if (ImGui::BeginPopupModal("Confirm Replace", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to replace:");
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", selectedAsset_.filename().string().c_str());
                ImGui::Text("with:");
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", replacementFilePath_.filename().string().c_str());
                ImGui::Separator();
                ImGui::TextWrapped("This action cannot be undone!");

                ImGui::Separator();

                if (ImGui::Button("Confirm", ImVec2(120, 0)))
                {
                    try
                    {
                        // Unload the texture from resource manager first
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tex")
                        {
                            resourceManager_->unload<Texture>(selectedAsset_.filename().string());
                        }

						else if (ext == ".ttf")
						{
							resourceManager_->unload<Font>(selectedAsset_.filename().string());
						}

						// Copy the replacement file over the original in the selected asset path
                        fs::copy(replacementFilePath_, selectedAsset_, fs::copy_options::overwrite_existing);
                        
                        // Copy to root
                        if (syncToRepo_)
                        {
                            fs::path repoDestinationPath;
                            // Get absolute path of the file we are replacing
                            std::string selectedAssetStr = fs::absolute(selectedAsset_).string();
                            std::string localAssetsStr = localAssetsPath_.string();
                            std::string localDataStr = localDataPath_.string();

                            // Check if the selected file is in the local 'Assets' folder tree
                            if (selectedAssetStr.find(localAssetsStr) == 0)
                            {
                                fs::path relativePath = fs::relative(selectedAsset_, localAssetsPath_);
                                repoDestinationPath = repoAssetsPath_ / relativePath;
                            }
                            // Check if the selected file is in the local 'Data' folder tree
                            else if (selectedAssetStr.find(localDataStr) == 0)
                            {
                                fs::path relativePath = fs::relative(selectedAsset_, localDataPath_);
                                repoDestinationPath = repoDataPath_ / relativePath;
                            }
                            else
                            {
                                Logger::Get().Info(DebugType::Editor)
                                    << "Selected asset is outside local 'Assets' or 'Data'. Skipping repo sync for " << selectedAsset_.filename().string() << std::endl;
                            }

                            // If we have a valid repo destination, copy the replacement file there
                            if (!repoDestinationPath.empty())
                            {
                                try
                                {
                                    // Ensure the destination directory exists
                                    fs::create_directories(repoDestinationPath.parent_path());
                                    // Copy the *replacement file* over the *repo file*
                                    fs::copy(replacementFilePath_, repoDestinationPath, fs::copy_options::overwrite_existing);
                                    Logger::Get().Info(DebugType::Editor)
                                        << "Synced (Repo) replacement to '" << repoDestinationPath.string() << "'" << std::endl;
                                }
                                catch (const fs::filesystem_error& e)
                                {
                                    Logger::Get().Error(DebugType::Editor)
                                        << "Failed to sync replacement file to repo: " << e.what() << std::endl;
                                }
                            }
                        }

                        Logger::Get().Info(DebugType::Editor)
                            << "Successfully replaced: " << selectedAsset_.string()
                            << " with " << replacementFilePath_.string() << std::endl;

                        // Reload the texture if it's an image
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                        {
                            if (resourceManager_->loadTexture(selectedAsset_.filename().string(), selectedAsset_.string()))
                            {
                                Logger::Get().Info(DebugType::Editor) << "Texture Key: " << selectedAsset_.filename().string() << std::endl;
                                Logger::Get().Info(DebugType::Editor) << "Reloaded texture after replacement" << std::endl;
                            }
                        }

                        // Refresh the directory view
                        ScanDirectory(currentPath_);
                    }
                    catch (const fs::filesystem_error& e)
                    {
                        Logger::Get().Error(DebugType::Editor)
                            << "Failed to replace file: " << e.what() << std::endl;
                    }

                    // Reset state
                    showReplaceConfirmation_ = false;
                    isReplaceModeActive_ = false;
                    replacementFilePath_.clear();

                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

				// Cancel button for replacement
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    showReplaceConfirmation_ = false;
                    replacementFilePath_.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

			// Delete Confirmation Modal
            if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to PERMANENTLY delete:");
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", selectedAsset_.filename().string().c_str());
                ImGui::Separator();
                ImGui::TextWrapped("This will remove the file from your disk and unload it from memory. This cannot be undone!");

                ImGui::Separator();

                if (ImGui::Button("Delete", ImVec2(120, 0)))
                {
                    try
                    {
                        std::string filename = selectedAsset_.filename().string();

                        // Unload from Resource Manager
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tex") {
                            resourceManager_->unload<Texture>(filename);
                        }
                        else if (ext == ".ttf") {
                            resourceManager_->unload<Font>(filename);
                        }
                        else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
                            resourceManager_->unload<FMOD::Sound>(filename); 
                        }
                        else if (ext == ".json") {
                            // Check if it is a loaded Prefab
                            if (resourceManager_->has<EntityPrefab>(filename)) {
                                resourceManager_->unload<EntityPrefab>(filename);
                                Logger::Get().Info(DebugType::Editor) << "Unloaded Prefab: " << filename << std::endl;
                            }
                            // Check if it is a loaded Animation
                            else if (resourceManager_->has<AnimationInfo>(filename)) {
                                resourceManager_->unload<AnimationInfo>(filename);
                                Logger::Get().Info(DebugType::Editor) << "Unloaded Animation: " << filename << std::endl;
                            }
                        }

                        // Repo Sync: Delete from repository if syncing is enabled
                        if (syncToRepo_)
                        {
                            fs::path repoDestinationPath;
                            std::string selectedAssetStr = fs::absolute(selectedAsset_).string();

                            if (selectedAssetStr.find(localAssetsPath_.string()) == 0) {
                                repoDestinationPath = repoAssetsPath_ / fs::relative(selectedAsset_, localAssetsPath_);
                            }
                            else if (selectedAssetStr.find(localDataPath_.string()) == 0) {
                                repoDestinationPath = repoDataPath_ / fs::relative(selectedAsset_, localDataPath_);
                            }

                            if (!repoDestinationPath.empty() && fs::exists(repoDestinationPath)) {
                                fs::remove(repoDestinationPath);
                                Logger::Get().Info(DebugType::Editor) << "Deleted file from Repo: " << repoDestinationPath.string() << std::endl;
                            }
                        }

                        // Delete local file
                        if (fs::remove(selectedAsset_)) {
                            Logger::Get().Info(DebugType::Editor) << "Successfully deleted local file: " << selectedAsset_.string() << std::endl;
                        }

                        // Reset state and refresh browser
                        selectedAsset_.clear();
                        ScanDirectory(currentPath_);
                    }
                    catch (const fs::filesystem_error& e)
                    {
                        Logger::Get().Error(DebugType::Editor) << "Failed to delete file: " << e.what() << std::endl;
                    }

                    showDeleteConfirmation_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    showDeleteConfirmation_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::Separator();

            // Asset preview
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                std::string texName = selectedAsset_.filename().string();

                // Check if the resource exists
                if (!resourceManager_->has<Texture>(texName))
                {
                    // If not, try to load it
                    resourceManager_->loadTexture(texName, selectedAsset_.string());
                }

                // Try to get resource
                try
                {
                    Texture& tex = resourceManager_->get<Texture>(texName);
                    ImGui::Image(tex, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
                }

                catch (const std::exception& e)
                {
                    Logger::Get().Info(DebugType::Editor) << "Failed to load file " << ": " << e.what() << std::endl;
                }
            }
            else {
                ImGui::Text("No preview available for this file type.");
            }
        }
        else {
            ImGui::Text("No asset selected.");
        }
    }

    ImGui::End();
}

// Handle files dropped into the asset browser
void AssetBrowser::HandleDroppedFiles(const std::vector<fs::path>& droppedFiles)
{
	// If no files were dropped, return early
    if (droppedFiles.empty()) return;

    Logger::Get().Info(DebugType::Editor) << "Processing " << droppedFiles.size() << " dropped files." << std::endl;

    bool filesCopied = false;
    for (const auto& sourcePath : droppedFiles)
    {
        try
        {
            if (!fs::is_regular_file(sourcePath)) {
                Logger::Get().Info(DebugType::Editor) << "Skipping non-file drop: " << sourcePath.string() << std::endl;
                continue; // Skip to the next file
            }

            fs::path filename = sourcePath.filename();

            // Copy to local path
            fs::path localDestinationPath = currentPath_ / filename;
            if (fs::exists(localDestinationPath)) {
                Logger::Get().Info(DebugType::Editor) << "File already exists, overwriting: " << localDestinationPath.string() << std::endl;
            }
            fs::copy(sourcePath, localDestinationPath, fs::copy_options::overwrite_existing);
            Logger::Get().Info(DebugType::Editor) << "Copied (Local) '" << filename.string() << "' to '" << localDestinationPath.string() << "'" << std::endl;
            filesCopied = true;

            std::string ext = sourcePath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); }); 

            if (ext == ".ttf")
            {
                // We assume resourceManager_ is valid, as it's used elsewhere
                if (resourceManager_)
                {
                    std::string fontKey = filename.string();
                    std::string fontPath = localDestinationPath.string();

                    // Load Font
                    if (resourceManager_->loadFont(fontKey, fontPath))
                    {
                        Logger::Get().Info(DebugType::Editor) << "Successfully loaded new font: " << fontKey << std::endl;
                    }
                    else
                    {
                        Logger::Get().Error(DebugType::Editor) << "Failed to load new font: " << fontKey << std::endl;
                    }
                }
            }

            if (syncToRepo_)
            {
                fs::path repoDestinationPath;
                std::string currentPathStr = fs::absolute(currentPath_).string();
                std::string localAssetsStr = localAssetsPath_.string();
                std::string localDataStr = localDataPath_.string();

                // Check if the drop location is within the local 'Assets' folder tree
                if (currentPathStr.find(localAssetsStr) == 0)
                {
                    fs::path relativePath = fs::relative(currentPath_, localAssetsPath_);
                    repoDestinationPath = repoAssetsPath_ / relativePath / filename;
                }
                // Check if the drop location is within the local 'Data' folder tree
                else if (currentPathStr.find(localDataStr) == 0)
                {
                    fs::path relativePath = fs::relative(currentPath_, localDataPath_);
                    repoDestinationPath = repoDataPath_ / relativePath / filename;
                }
                else
                {
                    Logger::Get().Info(DebugType::Editor)
                        << "Drop location is outside local 'Assets' or 'Data'. Skipping repo sync for " << filename.string() << std::endl;
                }

                // If we have a valid repo destination, create dirs and copy the file
                if (!repoDestinationPath.empty())
                {
                    try
                    {
                        fs::create_directories(repoDestinationPath.parent_path());
                        fs::copy(sourcePath, repoDestinationPath, fs::copy_options::overwrite_existing);
                        Logger::Get().Info(DebugType::Editor)
                            << "Copied (Repo) '" << filename.string() << "' to '" << repoDestinationPath.string() << "'" << std::endl;
                    }
                    catch (const fs::filesystem_error& e)
                    {
                        Logger::Get().Error(DebugType::Editor)
                            << "Failed to sync file to repo: " << e.what() << std::endl;
                    }
                }
            }
        }

        catch (const fs::filesystem_error& e)
        {
            Logger::Get().Error(DebugType::Editor) << "Failed to copy file locally: " << e.what() << std::endl;
        }
    }

    if (filesCopied)
    {
        ScanDirectory(currentPath_); // Refresh editor view
    }
}

// Handle file replacement
void AssetBrowser::HandleReplaceFile(const std::vector<fs::path>& droppedFiles)
{
    if (droppedFiles.empty() || !isReplaceModeActive_) return;

    // Get the first dropped file
    const fs::path& droppedFile = droppedFiles[0];

    // Validate that it's a regular file
    if (!fs::is_regular_file(droppedFile))
    {
        Logger::Get().Error(DebugType::Editor)
            << "Dropped item is not a file: " << droppedFile.string() << std::endl;
        return;
    }

    // Check if the file extensions match
    std::string selectedExt = selectedAsset_.extension().string();
    std::string droppedExt = droppedFile.extension().string();
    std::transform(selectedExt.begin(), selectedExt.end(), selectedExt.begin(),
        [](unsigned char c) { return static_cast<char>(::tolower(c)); });

    std::transform(droppedExt.begin(), droppedExt.end(), droppedExt.begin(),
        [](unsigned char c) { return static_cast<char>(::tolower(c)); });

	// If extensions do not match, log error and return
    if (selectedExt != droppedExt)
    {
        Logger::Get().Error(DebugType::Editor)
            << "File type mismatch! Selected: " << selectedExt
            << ", Dropped: " << droppedExt << std::endl;
        return;
    }

    // Store the replacement file path and show confirmation dialog
    replacementFilePath_ = droppedFile;
    showReplaceConfirmation_ = true;

    Logger::Get().Info(DebugType::Editor)
        << "Replacement file ready: " << droppedFile.string() << std::endl;
}