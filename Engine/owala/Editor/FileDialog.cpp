#include "FileDialog.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

namespace Editor {

    static fs::path currentDir;
    static std::string selectedFile;
    static bool dialogOpen = false;
    static bool confirmed = false;

    bool FileDialog::OpenFileDialog(const std::string& startPath, fs::path& outPath)
    {
        if (!dialogOpen) {
            currentDir = startPath.empty() ? fs::current_path() : fs::absolute(startPath);
            selectedFile.clear();
            dialogOpen = true;
            confirmed = false;
            ImGui::OpenPopup("Select File");
        }

        bool done = false;
        if (ImGui::BeginPopupModal("Select File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Current Directory: %s", currentDir.string().c_str());
            ImGui::Separator();

            // Go up one folder
            if (currentDir.has_parent_path()) {
                if (ImGui::Button("..")) {
                    currentDir = currentDir.parent_path();
                }
            }

            ImGui::BeginChild("FileList", ImVec2(500, 300), true);
            for (auto& entry : fs::directory_iterator(currentDir))
            {
                bool isDir = entry.is_directory();
                std::string name = entry.path().filename().string();

                if (isDir) {
                    if (ImGui::Selectable(("[DIR] " + name).c_str(), false))
                        currentDir = entry.path();
                }
                else {
                    if (ImGui::Selectable(name.c_str(), selectedFile == name))
                        selectedFile = name;
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (!selectedFile.empty())
                ImGui::Text("Selected: %s", selectedFile.c_str());
            else
                ImGui::Text("No file selected");

            if (ImGui::Button("OK")) {
                if (!selectedFile.empty()) {
                    outPath = currentDir / selectedFile;
                    confirmed = true;
                    done = true;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                done = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (done) {
            dialogOpen = false;
            return confirmed;
        }

        return false;
    }

} // namespace Editor
