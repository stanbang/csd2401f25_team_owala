/*****************************************************************//**
* \file   AssetViewerUI.cpp
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief    Contains the implementation of the UI rendering and interaction logic for the AssetBrowser, 
*           including icon loading, label truncation, and drag-and-drop file handling for asset importing and replacement.
*           
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "AssetBrowser.h"
#include "Logger.h"

using namespace Framework;

// Loads icon textures on demand and caches them for the session. Icons are stored in the project under "owala/Engine Icons".
void AssetBrowser::EnsureIconTexturesLoaded()
{
    // Icon textures are loaded lazily so startup stays fast.
    if (iconsLoaded_ || !resourceManager_) return;

    fs::path iconsDir = projectRootPath_ / "owala" / "Engine Icons";
    auto loadIcon = [&](const std::string& key, const std::string& fileName, Texture& outTex)
    {
        fs::path fullPath = iconsDir / fileName;
		// Check if the icon file exists before trying to load it, and log an error if it's missing. This helps catch issues with project setup or missing files.
        if (!fs::exists(fullPath))
        {
            Logger::Get().Error(DebugType::Editor) << "Missing icon: " << fullPath.string() << std::endl;
            return;
        }

		// Load the icon texture if it's not already in the resource manager. We use a unique key to avoid collisions with user assets.
        if (!resourceManager_->has<Texture>(key) && !resourceManager_->loadTexture(key, fullPath.string()))
        {
            Logger::Get().Error(DebugType::Editor) << "Failed to load icon: " << fullPath.string() << std::endl;
            return;
        }

        try
        {
            outTex = resourceManager_->get<Texture>(key);
        }
        catch (const std::exception& e)
        {
            Logger::Get().Error(DebugType::Editor) << "Failed to fetch icon texture (" << key << "): " << e.what() << std::endl;
        }
    };

    // The keys are explicit to avoid collisions with user asset keys.
    loadIcon("__engine_icon_folder__", "folder-solid.png", folderIconTexture_);
    loadIcon("__engine_icon_file__", "file-solid.png", fileIconTexture_);
    loadIcon("__engine_icon_font__", "font-solid.png", fontIconTexture_);
    loadIcon("__engine_icon_audio__", "music-solid.png", audioIconTexture_);
    iconsLoaded_ = true;
}

// Truncates a label to fit within a specified width, appending "..." if truncation occurs. Returns the truncated label and sets wasTruncated flag.
std::string AssetBrowser::TruncateLabelToFit(const std::string& text, float maxWidth, bool& wasTruncated) const
{
    wasTruncated = false;
    if (text.empty()) return text;

    if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;

    const std::string ellipsis = "...";
    if (ImGui::CalcTextSize(ellipsis.c_str()).x >= maxWidth)
    {
        wasTruncated = true;
        return ellipsis;
    }

    std::string out = text;
	// Iteratively remove characters from the end until the text with ellipsis fits within maxWidth. This is a simple approach; for very long texts, a more efficient method could be used.
    while (!out.empty())
    {
        out.pop_back();
        const std::string candidate = out + ellipsis;
        if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
        {
            wasTruncated = true;
            return candidate;
        }
    }

    wasTruncated = true;
    return ellipsis;
}

// Initializes the upload queue state for a new batch of dropped files. This resets all relevant flags and counters, and shows the upload progress popup.
void AssetBrowser::StartUploadQueue(const std::vector<fs::path>& droppedFiles)
{
    if (droppedFiles.empty()) return;

    // Reset upload session state every time we start a new batch.
    uploadQueue_ = droppedFiles;
    uploadInProgress_ = true;
    showUploadPopup_ = true;
    uploadHadErrors_ = false;
    uploadTotalFiles_ = uploadQueue_.size();
    uploadProcessedFiles_ = 0;
    uploadPopupDismissAt_ = 0.0;
    uploadCurrentFile_.clear();
}

// Copies a single dropped file to the local output directory and optionally syncs it to the repo mirror. Returns true on success, false on failure.
bool AssetBrowser::CopyDroppedFile(const fs::path& sourcePath)
{
    try
    {
        if (!fs::is_regular_file(sourcePath))
        {
            Logger::Get().Info(DebugType::Editor) << "Skipping non-file drop: " << sourcePath.string() << std::endl;
            return false;
        }
		// Copy the file to the current local directory first.
        fs::path filename = sourcePath.filename();
        fs::path localDestinationPath = currentPath_ / filename;
        if (fs::exists(localDestinationPath))
        {
            Logger::Get().Info(DebugType::Editor) << "File already exists, overwriting: " << localDestinationPath.string() << std::endl;
        }

        // Local copy always happens first; repo sync is a second step.
        fs::copy(sourcePath, localDestinationPath, fs::copy_options::overwrite_existing);
        Logger::Get().Info(DebugType::Editor) << "Copied (Local) '" << filename.string() << "' to '" << localDestinationPath.string() << "'" << std::endl;

		// ext is the file extension in lowercase, used for type-specific handling (e.g. auto-loading fonts). We normalize to lowercase to avoid case issues on different platforms.
        std::string ext = sourcePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

		// If the new file is a font, attempt to load it immediately so it's available in the editor without needing a restart. We use the filename as the resource key, which means users should avoid name collisions.
        if (ext == ".ttf" && resourceManager_)
        {
            std::string fontKey = filename.string();
            std::string fontPath = localDestinationPath.string();

            if (resourceManager_->loadFont(fontKey, fontPath))
            {
                Logger::Get().Info(DebugType::Editor) << "Successfully loaded new font: " << fontKey << std::endl;
            }
            else
            {
                Logger::Get().Error(DebugType::Editor) << "Failed to load new font: " << fontKey << std::endl;
            }
        }

		// If repo sync is enabled, determine the corresponding path in the repo mirror and copy the file there as well. This keeps the source project in sync with local changes made through the editor.
        if (syncToRepo_)
        {
            fs::path repoDestinationPath;
            std::string currentPathStr = fs::absolute(currentPath_).string();
            std::string localAssetsStr = localAssetsPath_.string();
            std::string localDataStr = localDataPath_.string();

			// Check if the current local path is within the 'Assets' or 'Data' tree, and construct the corresponding repo path. This ensures that files dropped into the editor are mirrored to the correct location in the source project.
            if (currentPathStr.find(localAssetsStr) == 0)
            {
                fs::path relativePath = fs::relative(currentPath_, localAssetsPath_);
                repoDestinationPath = repoAssetsPath_ / relativePath / filename;
            }
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

			// If we have a valid repo destination, copy the file there as well.
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

        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        Logger::Get().Error(DebugType::Editor) << "Failed to copy file locally: " << e.what() << std::endl;
        return false;
    }
}

// Processes one file from the upload queue per frame. This allows the editor to remain responsive during large batch uploads. When the queue is empty, it finalizes the upload session and refreshes the asset list.
void AssetBrowser::ProcessUploadQueue()
{
    if (!uploadInProgress_) return;

    // Process one file per frame to keep the editor responsive.
    if (uploadQueue_.empty())
    {
        uploadInProgress_ = false;
		uploadPopupDismissAt_ = ImGui::GetTime() + 1.0; // Show the final status for 1 second before allowing the popup to be dismissed.
		ScanDirectory(currentPath_); // Refresh the asset list to show the newly added files.
        return;
    }

    const fs::path sourcePath = uploadQueue_.front();
	uploadQueue_.erase(uploadQueue_.begin()); // Remove the file from the queue immediately to ensure progress is made even if copying fails.
	uploadCurrentFile_ = sourcePath.filename().string(); // Update the current file being processed for display in the upload progress popup.

    if (!CopyDroppedFile(sourcePath)) 
    {
        uploadHadErrors_ = true;
    }
    uploadProcessedFiles_++;
}

void AssetBrowser::RenderUI(std::vector<fs::path>& droppedFiles)
{
    EnsureIconTexturesLoaded();
    ProcessUploadQueue();

    if (ImGui::Begin("Project Explorer"))
    {
        if (ImGui::Button("Go to Root")) NavigateTo(rootPath_);
        ImGui::Separator();
        ShowFolderTree(rootPath_);
    }
    ImGui::End();

    if (ImGui::Begin("Asset Viewer"))
    {
        ImGui::TextWrapped("Directory: %s", currentPath_.string().c_str());
        ImGui::Text("Assets: %zu", currentAssets_.size());
        ImGui::Separator();

        ImGui::BeginChild("AssetList", ImVec2(0, 0), true);
        const ImVec2 childMin = ImGui::GetWindowPos();
        const ImVec2 childMax = ImVec2(childMin.x + ImGui::GetWindowSize().x, childMin.y + ImGui::GetWindowSize().y);

        if (!droppedFiles.empty() && !isReplaceModeActive_ &&
            (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow) || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow)))
        {
            if (uploadInProgress_)
            {
                // Allow new drops to be appended while an upload is still running.
                uploadQueue_.insert(uploadQueue_.end(), droppedFiles.begin(), droppedFiles.end());
                uploadTotalFiles_ += droppedFiles.size();
                showUploadPopup_ = true;
            }
            else
            {
                HandleDroppedFiles(droppedFiles);
            }
            droppedFiles.clear();
        }

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = 132.0f;
        int columns = std::max(1, static_cast<int>(availableWidth / itemWidth));
        ImGui::Columns(columns, "assetColumns", false);

        if (uploadInProgress_) ImGui::BeginDisabled();

        for (size_t i = 0; i < currentAssets_.size(); ++i)
        {
            const auto& asset = currentAssets_[i];
            ImGui::PushID(static_cast<int>(i));

            const ImVec2 thumbSize(96.0f, 96.0f);
            bool clicked = false;
            bool isImage = false;
            std::string ext;

            if (!asset.isDirectory)
            {
                ext = asset.path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
                isImage = (ext == ".png" || ext == ".jpg" || ext == ".jpeg");
            }

            ImGui::BeginGroup();

            if (isImage)
            {
                std::string texName = asset.path.filename().string();
                if (!resourceManager_->has<Texture>(texName))
                {
                    resourceManager_->loadTexture(texName, asset.path.string());
                }

                try
                {
                    Texture& tex = resourceManager_->get<Texture>(texName);
                    // Image assets show real thumbnail content.
                    clicked = ImGui::ImageButton(("asset_" + asset.name).c_str(), tex, thumbSize, ImVec2(0, 1), ImVec2(1, 0));
                }
                catch (const std::exception&)
                {
                    isImage = false;
                }
            }

            if (!isImage)
            {
                Texture iconTex = fileIconTexture_;
                if (asset.isDirectory) iconTex = folderIconTexture_;
                else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") iconTex = audioIconTexture_;
                else if (ext == ".ttf" || ext == ".otf") iconTex = fontIconTexture_;

                if (iconTex != 0)
                {
                    // Transparent background keeps icon colors accurate.
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.20f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 0.28f));
                    clicked = ImGui::ImageButton(("icon_" + asset.name).c_str(), iconTex, thumbSize, ImVec2(0, 1), ImVec2(1, 0));
                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();
                }
                else
                {
                    clicked = ImGui::Button("##assetIconFallback", thumbSize);
                }
            }

            if (clicked)
            {
                if (asset.isDirectory)
                {
					// Directory navigation logic: clicking ".." goes up, otherwise we navigate into the clicked folder.
                    if (asset.name == "..") NavigateUp();
                    else NavigateTo(asset.path);
                }
                else
                {
					// File selection logic (could be extended to open an editor, show details, etc.)
                    selectedAsset_ = asset.path;
                    Logger::Get().Info(DebugType::Editor) << "File selected: " << asset.path.string() << std::endl;
                }
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                const char* payloadType = "ASSET_PATH";
                currentDraggedPath_ = asset.path.string();
                ImGui::SetDragDropPayload(payloadType, currentDraggedPath_.c_str(),
                    (currentDraggedPath_.size() + 1) * sizeof(char));
                ImGui::Text("Dragging: %s", asset.name.c_str());
                ImGui::EndDragDropSource();
            }

            bool truncated = false;
            std::string shortLabel = TruncateLabelToFit(asset.name, thumbSize.x, truncated);
            ImGui::TextUnformatted(shortLabel.c_str());
            if (truncated && ImGui::IsItemHovered())
            {
                // Tooltip reveals the full name when ellipsis is shown.
                ImGui::SetTooltip("%s", asset.name.c_str());
            }

            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        if (uploadInProgress_) ImGui::EndDisabled();

        ImGui::Columns(1);

        if (uploadInProgress_)
        {
            // Block interactions with a translucent overlay while files are being copied.
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(childMin, childMax, IM_COL32(80, 80, 80, 120));
            const char* loadingText = "Uploading files...";
            ImVec2 textSize = ImGui::CalcTextSize(loadingText);
            ImVec2 center((childMin.x + childMax.x) * 0.5f, (childMin.y + childMax.y) * 0.5f);
            drawList->AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f), IM_COL32(240, 240, 240, 255), loadingText);
        }

        ImGui::EndChild();
    }
    ImGui::End();

    if (showUploadPopup_)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.95f);
        ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Upload Progress##AssetViewer", nullptr, popupFlags))
        {
			// Calculate progress as a value between 0.0 and 1.0, ensuring we don't divide by zero and clamping the result.
            float progress = (uploadTotalFiles_ > 0) ? (static_cast<float>(uploadProcessedFiles_) / static_cast<float>(uploadTotalFiles_)) : 1.0f;
            progress = std::clamp(progress, 0.0f, 1.0f);

			// The text changes based on the upload state.
            if (uploadInProgress_) ImGui::Text("Importing dropped files...");
            else if (uploadHadErrors_) ImGui::Text("Upload completed with errors");
            else ImGui::Text("Upload completed");

            ImGui::ProgressBar(progress, ImVec2(320.0f, 0.0f));
			ImGui::Text("%zu / %zu files", uploadProcessedFiles_, uploadTotalFiles_); // Show the current file being processed, truncated if it's too long, with a tooltip for the full name.
            if (!uploadCurrentFile_.empty())
            {
                bool currentNameTruncated = false;
                std::string currentName = TruncateLabelToFit(uploadCurrentFile_, 300.0f, currentNameTruncated);
                ImGui::Text("Current: %s", currentName.c_str());
                if (currentNameTruncated && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", uploadCurrentFile_.c_str());
                }
            }
        }
        ImGui::End();

		// Once the upload finishes, we keep the popup open for a short moment to show the final state before auto-closing it.
        if (!uploadInProgress_ && ImGui::GetTime() >= uploadPopupDismissAt_)
        {
            // Auto-close after a short delay so the user can read completion state.
            showUploadPopup_ = false;
        }
    }

    RenderAssetEditor(droppedFiles);
}

void AssetBrowser::HandleDroppedFiles(const std::vector<fs::path>& droppedFiles)
{
    if (droppedFiles.empty()) return;
    Logger::Get().Info(DebugType::Editor) << "Processing " << droppedFiles.size() << " dropped files." << std::endl;
	StartUploadQueue(droppedFiles); // This will manage the entire upload process, including progress tracking and UI updates.
}
