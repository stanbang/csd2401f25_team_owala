#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Editor {

    class FileDialog {
    public:
        static bool OpenFileDialog(const std::string& startPath, std::filesystem::path& outPath);
    };

} // namespace Editor