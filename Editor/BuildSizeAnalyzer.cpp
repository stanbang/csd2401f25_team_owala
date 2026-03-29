/*****************************************************************//**
* \file   BuildSizeAnalyzer.cpp
*
* \primary_author
*       Yar Xuan
*
* \brief	Implementation file for BuildSizeAnalyzer in the Editor module.
*			The BuildSizeAnalyzer is an editor tool that scans project directories, displays asset sizes,
*			and allows the user to export selected assets to a destination folder.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "pch.h"
#include "BuildSizeAnalyzer.h"
#include "FileUtility.h"
#include "Logger.h"
#include "imgui.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h> // For IFileOpenDialog
#endif

namespace fs = std::filesystem;

namespace Editor {

	// Converts raw bytes into a human-readable string (e.g., "1.50 MB").
	std::string BuildSizeAnalyzer::FormatBytes(uintmax_t bytes) const {
		const char* suffixes[] = { "B", "KB", "MB", "GB" };
		int suffixIndex = 0;
		double dBytes = static_cast<double>(bytes);
		while (dBytes >= 1024.0 && suffixIndex < 3) {
			dBytes /= 1024.0;
			suffixIndex++;
		}
		char buf[64];
		snprintf(buf, sizeof(buf), "%.2f %s", dBytes, suffixes[suffixIndex]);
		return std::string(buf);
	}

	// Recalculates the totalSelectedBuildSize_ by summing the sizes of checked items.
	void BuildSizeAnalyzer::CalculateTotalSelectedBuildSize() {
		totalSelectedBuildSize_ = 0;
		for (const BuildAssetItem& asset : buildAssets_) {
			if (asset.selected) {
				totalSelectedBuildSize_ += asset.size;
			}
		}
	}

	// Scans the source directory for files and updates the buildAssets_ list.
	void BuildSizeAnalyzer::RefreshBuildAssets() {
		// Remember previous selections if the window is reopened/refreshed
		std::unordered_map<std::string, bool> previousSelections;
		for (const BuildAssetItem& asset : buildAssets_) {
			previousSelections[asset.relativePath] = asset.selected;
		}
		buildAssets_.clear();

		std::string assetsDir = Framework::FileIOSystem::GetSourceAssetsDir();
		if (assetsDir.empty()) assetsDir = "Assets";

		if (fs::exists(assetsDir)) {
			for (const fs::directory_entry& entry : fs::recursive_directory_iterator(assetsDir)) {
				if (entry.is_regular_file()) {
					BuildAssetItem item;
					item.relativePath = fs::relative(entry.path(), assetsDir).generic_string();
					item.filename = entry.path().filename().string();
					item.size = fs::file_size(entry.path());

					// Restore selection state, defaulting to true for newly discovered assets
					auto it = previousSelections.find(item.relativePath);
					item.selected = (it != previousSelections.end()) ? it->second : true;

					buildAssets_.push_back(item);
				}
			}
		}
		CalculateTotalSelectedBuildSize();
	}

#ifdef _WIN32
	// Native Windows helper to open the standard folder picker dialog.
	std::string BuildSizeAnalyzer::OpenFolderBrowser() {
		std::string result = "";

		// Initialize COM library
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		bool needUninit = SUCCEEDED(hr);

		IFileOpenDialog* pFileOpen;
		// Create the FileOpenDialog object
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr)) {
			DWORD dwOptions;
			if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
				// Set the option to pick folders instead of files
				pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			}

			// Show the dialog
			if (SUCCEEDED(pFileOpen->Show(NULL))) {
				IShellItem* pItem;
				if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
					PWSTR pszFilePath;
					// Get the file system path
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
						// Convert wide string (PWSTR) to std::string
						int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
						if (size_needed > 0) {
							std::string strTo(size_needed - 1, 0); // -1 to exclude null terminator
							WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &strTo[0], size_needed, NULL, NULL);
							result = strTo;
						}
						CoTaskMemFree(pszFilePath); // Free the memory allocated by the shell
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}

		if (needUninit) {
			CoUninitialize();
		}

		return result;
	}
#endif

	// Copies all selected assets to the destination folder specified in exportDestinationBuffer_.
	void BuildSizeAnalyzer::ExportSelectedBuildAssets() {
		std::string destDir = exportDestinationBuffer_;

		// Validate destination path
		if (destDir.empty()) {
			ImGui::OpenPopup("Export Error");
			Framework::Logger::Get().Error(DebugType::Editor) << "Export destination path is empty!" << std::endl;
			return;
		}

		std::string sourceDir = Framework::FileIOSystem::GetSourceAssetsDir();
		if (sourceDir.empty()) sourceDir = "Assets";

		try {
			// Iterate through cached list and copy files
			for (const BuildAssetItem& asset : buildAssets_) {
				if (asset.selected) {
					// Construct full absolute paths
					fs::path src = fs::path(sourceDir) / asset.relativePath;
					fs::path dst = fs::path(destDir) / asset.relativePath;

					// Ensure the target subdirectory exists before copying
					fs::create_directories(dst.parent_path());

					// Copy the file, overwriting it if it already exists at the destination
					fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
				}
			}

			// Trigger the success modal in ImGui
			ImGui::OpenPopup("Export Successful");
			Framework::Logger::Get().Info(DebugType::Editor) << "Exported selected assets to: " << destDir << std::endl;
		}
		catch (const std::exception& e) {
			Framework::Logger::Get().Error(DebugType::Editor) << "Asset Export failed: " << e.what() << std::endl;
		}
	}

	// Renders the ImGui window and all its internal UI components.
	void BuildSizeAnalyzer::ShowUI() {
		if (!showBuildSizeAnalyzer_) return;

		// Refresh assets only when the window is opened for the very first time
		if (!buildAnalyzerInitialized_) {
			RefreshBuildAssets();
			buildAnalyzerInitialized_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Build Size Analyzer", &showBuildSizeAnalyzer_)) {

			// --- TOP CONTROL BAR ---
			if (ImGui::Button("Refresh Assets")) {
				RefreshBuildAssets();
			}
			ImGui::SameLine();
			if (ImGui::Button("Select All")) {
				for (auto& asset : buildAssets_) asset.selected = true;
				CalculateTotalSelectedBuildSize();
			}
			ImGui::SameLine();
			if (ImGui::Button("Deselect All")) {
				for (auto& asset : buildAssets_) asset.selected = false;
				CalculateTotalSelectedBuildSize();
			}

			ImGui::Separator();

			// --- HIGHLIGHT TOTAL SIZE ---
			ImGui::Text("Total Export Size: ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", FormatBytes(totalSelectedBuildSize_).c_str());

			ImGui::Separator();

			// --- ASSET LIST TABLE ---
			if (ImGui::BeginChild("AssetList", ImVec2(0, -70), true)) {
				if (ImGui::BeginTable("BuildAssetsTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
					ImGui::TableSetupColumn("Export", ImGuiTableColumnFlags_WidthFixed, 50.0f);
					ImGui::TableSetupColumn("Asset Path", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableHeadersRow();

					for (auto& asset : buildAssets_) {
						ImGui::TableNextRow();

						// Column 1: Checkbox
						ImGui::TableSetColumnIndex(0);
						ImGui::PushID(asset.relativePath.c_str());
						if (ImGui::Checkbox("##sel", &asset.selected)) {
							CalculateTotalSelectedBuildSize();
						}
						ImGui::PopID();

						// Column 2: File Path
						ImGui::TableSetColumnIndex(1);
						ImGui::TextUnformatted(asset.relativePath.c_str());

						// Column 3: Formatted Size
						ImGui::TableSetColumnIndex(2);
						ImGui::TextUnformatted(FormatBytes(asset.size).c_str());
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();

			// --- BOTTOM EXPORT CONTROLS ---
			ImGui::Text("Destination:");
			ImGui::SameLine();

			// Input Text Field
			ImGui::SetNextItemWidth(-100.0f);
			ImGui::InputText("##ExportDest", exportDestinationBuffer_, sizeof(exportDestinationBuffer_));
			
#ifdef _WIN32
			// Browse Button (Opens Windows Explorer)
			ImGui::SameLine();
			if (ImGui::Button("Browse...")) {
				std::string selectedFolder = OpenFolderBrowser();
				if (!selectedFolder.empty()) {
					// Copy the selected path into the ImGui buffer safely
					strncpy_s(exportDestinationBuffer_, sizeof(exportDestinationBuffer_), selectedFolder.c_str(), _TRUNCATE);
				}
			}
#endif
			// Final Export Button
			if (ImGui::Button("Export", ImVec2(80, 0))) {
				ExportSelectedBuildAssets();
			}

			// --- MODALS ---
		
			// Simple success feedback modal
			if (ImGui::BeginPopupModal("Export Successful", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Assets successfully exported to:");
				ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", exportDestinationBuffer_);
				ImGui::Spacing();
				if (ImGui::Button("Close", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			// Simple error feedback modal
			if (ImGui::BeginPopupModal("Export Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Export Destination path is empty!");
				ImGui::Spacing();
				if (ImGui::Button("Close", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

}