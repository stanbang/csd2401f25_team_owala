#pragma once
#include "AssetBrowser.h"
#include "Logger.h"

using namespace Framework;

void AssetBrowser::Initialize(const fs::path& rootPath)
{
    // Validate input path
    if (rootPath.empty()) {
        Logger::Get().Error(DebugType::Editor) << "AssetBrowser: Empty root path provided!" << std::endl;
        return;
    }

    rootPath_ = fs::absolute(rootPath);
    currentPath_ = rootPath_;

    Logger::Get().Info(DebugType::Editor) << "AssetBrowser initializing with root: '" << rootPath_.string() << "'" << std::endl;

    if (fs::exists(rootPath_) && fs::is_directory(rootPath_))
        ScanDirectory(rootPath_);
    else
        Logger::Get().Error(DebugType::Editor) << "AssetBrowser: Root path invalid: " << rootPath_.string() << std::endl;
}

void AssetBrowser::ScanDirectory(const fs::path& directoryPath)
{
    // LOG THE INCOMING PATH FIRST
    Logger::Get().Info(DebugType::Editor) << "ScanDirectory called with: '" << directoryPath.string() << "'" << std::endl;

    // Validate the path before scanning
    if (directoryPath.empty()) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Empty path provided!" << std::endl;
        return;
    }

    if (!fs::exists(directoryPath)) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Path does not exist: '" << directoryPath.string() << "'" << std::endl;
        return;
    }

    if (!fs::is_directory(directoryPath)) {
        Logger::Get().Error(DebugType::Editor) << "ScanDirectory: Path is not a directory: '" << directoryPath.string() << "'" << std::endl;
        return;
    }

    currentAssets_.clear();
    currentPath_ = directoryPath;
    Logger::Get().Info(DebugType::Editor) << "Assigned currentPath_: '" << currentPath_.string() << "'" << std::endl;

    // Parent folder entry
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

    try
    {
        int fileCount = 0;
        int dirCount = 0;

        for (const auto& entry : fs::directory_iterator(currentPath_))
        {
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

void AssetBrowser::NavigateTo(const fs::path& target)
{
    Logger::Get().Info(DebugType::Editor) << "NavigateTo called with: '" << target.string() << "'" << std::endl;

    if (target.empty()) {
        Logger::Get().Error(DebugType::Editor) << "NavigateTo: Empty path provided!" << std::endl;
        return;
    }

    if (!fs::exists(target)) {
        Logger::Get().Error(DebugType::Editor) << "NavigateTo: Path does not exist: '" << target.string() << "'" << std::endl;
        return;
    }

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

void AssetBrowser::NavigateUp()
{
    Logger::Get().Info(DebugType::Editor) << "NavigateUp called. Current: '" << currentPath_.string() << "', Root: '" << rootPath_.string() << "'" << std::endl;

    if (currentPath_ == rootPath_) {
        Logger::Get().Info(DebugType::Editor) << "Already at root, cannot navigate up" << std::endl;
        return;
    }

    fs::path parent = currentPath_.parent_path();
    Logger::Get().Info(DebugType::Editor) << "Parent path: '" << parent.string() << "'" << std::endl;

    // Check if parent is within root
    std::string parentStr = parent.string();
    std::string rootStr = rootPath_.string();

    if (parentStr.find(rootStr) != 0) {
        Logger::Get().Info(DebugType::Editor) << "Parent outside root, navigating to root instead" << std::endl;
        parent = rootPath_;
    }

    ScanDirectory(parent);
}

void AssetBrowser::ShowFolderTree(const fs::path& path)
{
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }

    try {
        for (const auto& entry : fs::directory_iterator(path))
        {
            const std::string name = entry.path().filename().string();
            bool isDir = entry.is_directory();
            ImGuiTreeNodeFlags flags = isDir ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf;

            bool opened = ImGui::TreeNodeEx(name.c_str(), flags);

            if (ImGui::IsItemClicked() && !isDir)
            {
                Logger::Get().Info(DebugType::Editor) << "File clicked: " << entry.path().string() << std::endl;
            }
            else if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0) && isDir)
            {
                Logger::Get().Info(DebugType::Editor) << "Folder double-clicked: " << entry.path().string() << std::endl;
                NavigateTo(entry.path());
            }

            if (opened && isDir)
            {
                ShowFolderTree(entry.path());
                ImGui::TreePop();
            }
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

void AssetBrowser::RenderUI(std::vector<fs::path>& droppedFiles)
{
    // --- Left: Folder Tree ---
    if (ImGui::Begin("Project Explorer"))
    {
        if (ImGui::Button("Go to Root"))
            NavigateTo(rootPath_);

        ImGui::Separator();
        ShowFolderTree(rootPath_);
    }
    ImGui::End();

    // --- Right: File Contents ---
    if (ImGui::Begin("Asset Viewer"))
    {
        ImGui::TextWrapped("Directory: %s", currentPath_.string().c_str());
        ImGui::Text("Assets: %zu", currentAssets_.size());
        ImGui::Separator();

        ImGui::BeginChild("AssetList", ImVec2(0, 0), true);
        
        if (!droppedFiles.empty() && !isReplaceModeActive_ &&
            (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow) ||
                ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow)))
        {
            HandleDroppedFiles(droppedFiles);
            droppedFiles.clear();
        }

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = 120.0f;
        int columns = std::max(1, (int)(availableWidth / itemWidth));
        ImGui::Columns(columns, "assetColumns", false);

        for (size_t i = 0; i < currentAssets_.size(); ++i)
        {
            const auto& asset = currentAssets_[i];
            ImGui::PushID(static_cast<int>(i));

            ImVec2 itemSize(100, 100);

            if (asset.isDirectory)
            {
                std::string displayName = "[DIR] " + asset.name;
                if (ImGui::Button(displayName.c_str(), itemSize))
                {
                    if (asset.name == "..")
                        NavigateUp();
                    else
                        NavigateTo(asset.path);
                }
            }
            else
            {
                std::string ext = asset.path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                bool clicked = false;

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                {
                    std::string texName = asset.path.filename().string();

                    // 1. Check if the resource exists
                    if (!resourceManager_->has<Texture>(texName))
                    {
                        // 2. If not, try to load it
                        resourceManager_->loadTexture(texName, asset.path.string());
                    }

                    // 3. Now, try to get it.
                    //    (Still use try/catch in case loading failed and it's *still* not there)
                    try
                    {
                        Texture& tex = resourceManager_->get<Texture>(texName);
                        clicked = ImGui::ImageButton(("asset_" + asset.name).c_str(), tex, itemSize, ImVec2(0, 1), ImVec2(1, 0));
                    }

                    catch (const std::exception& e)
                    {
                        Logger::Get().Info(DebugType::Editor) << "Failed to load file" << std::endl;
                    }
                }

                if (clicked)
                {
                    selectedAsset_ = asset.path;
                    Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
                }
                else if (!(ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                {
                    if (ImGui::Button(asset.display.c_str(), itemSize))
                    {
                        selectedAsset_ = asset.path;
                        Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
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

    RenderAssetEditor(droppedFiles);
}

void AssetBrowser::RenderAssetEditor(std::vector<fs::path>& droppedFiles)
{
    if (ImGui::Begin("Asset Editor")) {
        // If selected asset is not empty, aka something is selected
        if (!selectedAsset_.empty()) {
            ImGui::TextWrapped("Selected Asset: %s", selectedAsset_.string().c_str());
            ImGui::Separator();
            std::string ext = selectedAsset_.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ImGui::Button("Reimport Asset", ImVec2(150, 30)))
            {
                Logger::Get().Info(DebugType::Editor) << "Reimporting asset: " << selectedAsset_.string() << std::endl;

                try
                {
                    // Unload old texture if needed
                    resourceManager_->unload<Texture>(selectedAsset_.string());

                    // Reload texture
                    if (resourceManager_->loadTexture(selectedAsset_.string(), selectedAsset_.string()))
                    {
                        Logger::Get().Info(DebugType::Editor) << "Reimport successful for: " << selectedAsset_.string() << std::endl;
                    }
                    else
                    {
                        Logger::Get().Error(DebugType::Editor) << "Reimport failed for: " << selectedAsset_.string() << std::endl;
                    }
                }
                catch (const std::exception& e)
                {
                    Logger::Get().Error(DebugType::Editor) << "Reimport exception: " << e.what() << std::endl;
                }
            }

            // Replace Button
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

            // Confirmation Popup
            if (showReplaceConfirmation_)
            {
                ImGui::OpenPopup("Confirm Replace");
            }

            // Confirmation Modal
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
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                        {
                            resourceManager_->unload<Texture>(selectedAsset_.string());
                        }

						// Copy the replacement file over the original in the selected asset path
                        fs::copy(replacementFilePath_, selectedAsset_, fs::copy_options::overwrite_existing);
                        


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

                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    showReplaceConfirmation_ = false;
                    replacementFilePath_.clear();
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
                    Logger::Get().Info(DebugType::Editor) << "Failed to load file" << std::endl;
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

void AssetBrowser::HandleDroppedFiles(const std::vector<fs::path>& droppedFiles)
{
    if (droppedFiles.empty()) return;

    Logger::Get().Info(DebugType::Editor) << "Processing " << droppedFiles.size() << " dropped files." << std::endl;

    bool filesCopied = false;
    for (const auto& sourcePath : droppedFiles)
    {
        try
        {
            // Only copy if it's a regular file
            if (fs::is_regular_file(sourcePath))
            {
                fs::path destinationPath = currentPath_ / sourcePath.filename();

                // Check if file already exists and handle accordingly
                if (fs::exists(destinationPath))
                {
                    Logger::Get().Info(DebugType::Editor)
                        << "File already exists, overwriting: " << destinationPath.string() << std::endl;
                }

                fs::copy(sourcePath, destinationPath, fs::copy_options::overwrite_existing);

                Logger::Get().Info(DebugType::Editor)
                    << "Copied '" << sourcePath.string() << "' to '" << destinationPath.string() << "'" << std::endl;
                filesCopied = true;
            }
            else
            {
                Logger::Get().Info(DebugType::Editor)
                    << "Skipping non-file drop: " << sourcePath.string() << std::endl;
            }
        }
        catch (const fs::filesystem_error& e)
        {
            Logger::Get().Error(DebugType::Editor)
                << "Failed to copy file: " << e.what() << std::endl;
        }
    }

    // If we successfully copied anything, refresh the asset list
    if (filesCopied)
    {
        ScanDirectory(currentPath_);
    }
}

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
    std::transform(selectedExt.begin(), selectedExt.end(), selectedExt.begin(), ::tolower);
    std::transform(droppedExt.begin(), droppedExt.end(), droppedExt.begin(), ::tolower);

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