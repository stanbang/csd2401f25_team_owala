#pragma once
#include "AssetBrowser.h"
#include <imgui.h>
#include <algorithm>
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

void AssetBrowser::RenderUI()
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
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = 120.0f;
        int columns = std::max(1, (int)(availableWidth / itemWidth));
        ImGui::Columns(columns, "assetColumns", false);

        for (size_t i = 0; i < currentAssets_.size(); ++i)
        {
            const auto& asset = currentAssets_[i];
            ImGui::PushID(static_cast<int>(i));

            ImTextureID texID = 0;

            // Only try to load preview for image files
            if (!asset.isDirectory) {
                std::string ext = asset.path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                    try {
                        Texture& tex = resourceManager_->get<Texture>(asset.path.string());
                        texID = (ImTextureID)(intptr_t)tex;
                    }
                    catch (const std::out_of_range&) {
                        // Texture not loaded yet, try loading it
                        if (resourceManager_->loadTexture(asset.path.string(), asset.path.string())) {
                            Texture& tex = resourceManager_->get<Texture>(asset.path.string());
                            texID = (ImTextureID)(intptr_t)tex;
                        }
                    }
                }
            }

            ImVec2 itemSize(100, 100);

            if (asset.isDirectory) {
                ImGui::Button("[DIR]", itemSize);
            }
            else if (texID) {
                ImGui::Image(texID, itemSize);
            }
            else {
                ImGui::Button(asset.display.c_str(), itemSize);
            }

            // Handle clicks
            if (ImGui::IsItemClicked()) {
                if (asset.name == "..")
                    NavigateUp();
                else if (asset.isDirectory)
                    NavigateTo(asset.path);
                else
                    Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::EndChild();
    }
    ImGui::End();
}