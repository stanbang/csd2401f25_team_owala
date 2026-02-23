/*****************************************************************//**
 * \file   AnimationEditor.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implementation of animation file loading, saving and editing in the
 *			IMGUI editor.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "Editor.h"
#include "Logger.h"
#include "FileUtility.h"

namespace Editor {

	void Editor::LoadAnimationFiles() {
		using namespace Framework;

		loadedAnimations_.clear();
		animationNames_.clear();

		try {
			// Look for animation JSON files in Data/AnimationData folder
			std::string animDir = "Data/AnimationData/";
			if (std::filesystem::exists(animDir)) {
				for (const auto& entry : std::filesystem::directory_iterator(animDir)) {
					if (entry.is_regular_file() && entry.path().extension() == ".json") {
						std::string filename = entry.path().stem().string();

						// Try to load animation data from file
						rapidjson::Document doc;
						if (FileIOSystem::ReadFile(entry.path().string(), doc)) {
							if (doc.IsObject()) {
								AnimationFileData animData;
								animData.name = filename;

							if (doc.HasMember(FileIOKeys::FRAME_COUNT)) {
								animData.frameCount = doc[FileIOKeys::FRAME_COUNT].GetInt();
							}
							if (doc.HasMember(FileIOKeys::FRAME_DURATION)) {
								animData.frameDuration = doc[FileIOKeys::FRAME_DURATION].GetFloat();
							}
							if (doc.HasMember(FileIOKeys::IS_LOOPING)) {
								animData.isLooping = doc[FileIOKeys::IS_LOOPING].GetBool();
							}
							if (doc.HasMember(FileIOKeys::START_FRAME)) {
								animData.startFrame = doc[FileIOKeys::START_FRAME].GetInt();
							}

							// Load spritesheets
							if (doc.HasMember("spritesheets") && doc["spritesheets"].IsArray()) {
								const rapidjson::Value& sheetsJson = doc["spritesheets"];
								for (rapidjson::SizeType i = 0; i < sheetsJson.Size(); ++i) {
										const rapidjson::Value& sheet = sheetsJson[i];
										if (sheet.HasMember("texture") && sheet.HasMember("count")) {
											animData.spritesheets.push_back({
												sheet["texture"].GetString(),
												sheet["count"].GetInt()
											});
										}
									}
								}

								loadedAnimations_.push_back(animData);

								Logger::Get().Info(DebugType::Editor)
									<< "Loaded animation: " << filename << " (frameCount: " << animData.frameCount << ")" << std::endl;
							}
						}
					}
				}
			}
			else {
				Logger::Get().Error(DebugType::Editor)
					<< "Animation directory not found: " << animDir << std::endl;
			}
		}
		catch (const std::exception& e) {
			Logger::Get().Error(DebugType::Editor)
				<< "Error loading animation files: " << e.what() << std::endl;
		}

		// Build the const char* array for ImGui using stable storage
		animationNames_.clear();
		animationNames_.push_back("-- Select Animation --");
		for (const auto& anim : loadedAnimations_) {
			animationNames_.push_back(anim.name.c_str());
		}
	}

	void Editor::SaveAnimationFile(const std::string& animationName) {
		using namespace Framework;

		// Find the animation in our loaded list
		int animIndex = -1;
		for (size_t i = 0; i < loadedAnimations_.size(); ++i) {
			if (loadedAnimations_[i].name == animationName) {
				animIndex = static_cast<int>(i);
				break;
			}
		}

		if (animIndex < 0) {
			Logger::Get().Error(DebugType::Editor)
				<< "Animation not found: " << animationName << std::endl;
			return;
		}

		AnimationFileData& animData = loadedAnimations_[animIndex];

	// Create JSON document
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

	doc.AddMember(rapidjson::StringRef(FileIOKeys::FRAME_COUNT), animData.frameCount, alloc);
	doc.AddMember(rapidjson::StringRef(FileIOKeys::FRAME_DURATION), animData.frameDuration, alloc);
	doc.AddMember(rapidjson::StringRef(FileIOKeys::IS_LOOPING), animData.isLooping, alloc);
	doc.AddMember(rapidjson::StringRef(FileIOKeys::START_FRAME), animData.startFrame, alloc);

	// Add spritesheets array
	rapidjson::Value sheetsArray(rapidjson::kArrayType);
	for (const auto& sheet : animData.spritesheets) {
		rapidjson::Value sheetObj(rapidjson::kObjectType);
		sheetObj.AddMember("texture", rapidjson::StringRef(sheet.first.c_str()), alloc);
		sheetObj.AddMember("count", sheet.second, alloc);
		sheetsArray.PushBack(sheetObj, alloc);
	}
	doc.AddMember("spritesheets", sheetsArray, alloc);

		// Write to Debug folder first
		std::string debugFilepath = "Data/AnimationData/" + animationName + ".json";
		bool debugSaveSuccess = false;
		try {
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);

			std::ofstream file(debugFilepath);
			if (file.is_open()) {
				file << buffer.GetString();
				file.close();
				debugSaveSuccess = true;

				Logger::Get().Info(DebugType::Editor)
					<< "Saved animation file: " << debugFilepath << std::endl;
			}
			else {
				Logger::Get().Error(DebugType::Editor)
					<< "Failed to open file for writing: " << debugFilepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			Logger::Get().Error(DebugType::Editor)
				<< "Error saving animation file to Debug: " << e.what() << std::endl;
		}

		// Also save to root Data folder - using correct relative path
		// From Debug folder, we need to go up: ../../Data/AnimationData/
		std::string rootFilepath = "../../Data/AnimationData/" + animationName + ".json";
		try {
			// Ensure the directory exists
			std::filesystem::path rootPath(rootFilepath);
			std::filesystem::create_directories(rootPath.parent_path());

			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);

			std::ofstream file(rootFilepath);
			if (file.is_open()) {
				file << buffer.GetString();
				file.close();

				Logger::Get().Info(DebugType::Editor)
					<< "Saved animation file to root: " << rootFilepath << std::endl;
			}
			else {
				Logger::Get().Error(DebugType::Editor)
					<< "Failed to open file for writing to root: " << rootFilepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			Logger::Get().Error(DebugType::Editor)
				<< "Error saving animation file to root: " << e.what() << std::endl;
		}

		// Reload animation from file into ResourceManager if successfully saved
		if (debugSaveSuccess) {
			RefreshAnimationInResourceManager(animationName);
		}

		Logger::Get().Info(DebugType::Editor)
			<< "Animation saved and resource manager refreshed: " << animationName << std::endl;
	}

	void Editor::RefreshAnimationInResourceManager(const std::string& animationName) {
		using namespace Framework;

		if (!resourceManager_) {
			Logger::Get().Error(DebugType::Editor)
				<< "Failed to access ResourceManager for animation refresh" << std::endl;
			return;
		}

		std::string filepath = "Data/AnimationData/" + animationName + ".json";
		std::vector<std::string> textureKeysToRefresh;
		
		// Find all texture keys that reference this animation by checking the mapping file
		try {
			rapidjson::Document mappingDoc;
			if (FileIOSystem::ReadFile("Data/textureToAnimationMapping.json", mappingDoc)) {
				if (mappingDoc.IsObject() && mappingDoc.HasMember("animations") && mappingDoc["animations"].IsArray()) {
					const rapidjson::Value& animations = mappingDoc["animations"];
					for (rapidjson::SizeType i = 0; i < animations.Size(); ++i) {
						const rapidjson::Value& entry = animations[i];
						if (entry.HasMember("animation_path") && entry["animation_path"].IsString()) {
							std::string animPath = entry["animation_path"].GetString();
							if (animPath.find(animationName) != std::string::npos) {
								if (entry.HasMember("texture_keys") && entry["texture_keys"].IsArray()) {
									const rapidjson::Value& keys = entry["texture_keys"];
									for (rapidjson::SizeType j = 0; j < keys.Size(); ++j) {
										if (keys[j].IsString()) {
											textureKeysToRefresh.push_back(keys[j].GetString());
										}
									}
								}
								else if (entry.HasMember("texture_key") && entry["texture_key"].IsString()) {
									textureKeysToRefresh.push_back(entry["texture_key"].GetString());
								}
							}
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			Logger::Get().Error(DebugType::Editor) << "Error reading texture-to-animation mapping: " << e.what() << std::endl;
		}
		
		// If no texture keys found, use animation name as fallback
		if (textureKeysToRefresh.empty()) {
			textureKeysToRefresh.push_back(animationName);
		}

		// Remove and reload all affected texture keys
		auto& animationMap = resourceManager_->getAnimationMap();
		for (const auto& textureKey : textureKeysToRefresh) {
			auto it = animationMap.find(textureKey);
			if (it != animationMap.end()) {
				animationMap.erase(it);
				Logger::Get().Info(DebugType::Editor) << "Removed old animation from cache: " << textureKey << std::endl;
			}
			if (resourceManager_->loadAnimationInfo(textureKey, filepath)) {
				Logger::Get().Info(DebugType::Editor) << "Refreshed animation in ResourceManager for texture key: " << textureKey << std::endl;
			}
		}
	}

	void Editor::ShowAnimationEditor() {
		using namespace Framework;

		// Initialize animation list on first show
		if (!isAnimationEditorInitialized_) {
			LoadAnimationFiles();
			isAnimationEditorInitialized_ = true;
		}

		ImGui::Begin("Animation Editor", &settings.showAnimationEditor);

		ImGui::Text("Animation File Editor");
		ImGui::Separator();

		//Animation Selection
		ImGui::Text("Select Animation:");
		ImGui::SetNextItemWidth(250.0f);
		if (ImGui::Combo("##AnimationList", &selectedAnimationIndex_, animationNames_.data(), 
			static_cast<int>(animationNames_.size()))) {
			if (selectedAnimationIndex_ > 0) {
				selectedAnimationName_ = loadedAnimations_[selectedAnimationIndex_ - 1].name;
			}
			else {
				selectedAnimationName_.clear();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Refresh List")) {
			LoadAnimationFiles();
			selectedAnimationIndex_ = 0;
			selectedAnimationName_.clear();
		}

		ImGui::Separator();

		// --- Animation Editor Content ---
		if (!selectedAnimationName_.empty() && selectedAnimationIndex_ > 0) {
			AnimationFileData& animData = loadedAnimations_[selectedAnimationIndex_ - 1];

			ImGui::Text("Editing: %s", animData.name.c_str());
			ImGui::Spacing();

			// --- Frame Count ---
			ImGui::InputInt("Frame Count##anim", &animData.frameCount);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Total number of frames in this animation");
			}

			// --- Frame Duration ---
			ImGui::InputFloat("Frame Duration (seconds)##anim", &animData.frameDuration, 0.01f, 0.1f, "%.3f");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Time to display each frame (in seconds)");
			}

			// --- Looping ---
			ImGui::Checkbox("Is Looping##anim", &animData.isLooping);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Whether this animation loops when it reaches the last frame");
			}

			// --- Start Frame ---
			ImGui::InputInt("Start Frame##anim", &animData.startFrame);
			animData.startFrame = std::max(0, std::min(animData.startFrame, animData.frameCount - 1));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Which frame to start the animation on (0-indexed)");
			}

			ImGui::Separator();
			ImGui::SeparatorText("Spritesheets");

			// Display current spritesheets
			if (!animData.spritesheets.empty()) {
				if (ImGui::BeginTable("SpriteSheetTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Texture Key", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Frame Count", ImGuiTableColumnFlags_WidthFixed, 120.0f);
					ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableHeadersRow();

					for (size_t i = 0; i < animData.spritesheets.size(); ++i) {
						ImGui::PushID(static_cast<int>(i));
						ImGui::TableNextRow();

						// Texture Key
						ImGui::TableNextColumn();
						ImGui::Text("%s", animData.spritesheets[i].first.c_str());

						// Frame Count
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1.0f);
						ImGui::InputInt("##FrameCount", &animData.spritesheets[i].second);

						// Remove Button
						ImGui::TableNextColumn();
						if (ImGui::Button("Remove##sheet", ImVec2(-1.0f, 0.0f))) {
							animData.spritesheets.erase(animData.spritesheets.begin() + i);
						}

						ImGui::PopID();
					}

					ImGui::EndTable();
				}
			}
			else {
				ImGui::TextDisabled("No spritesheets added yet");
			}

			// Add new spritesheet
			ImGui::Spacing();
			static char newSheetTexture[256] = "";
			static int newSheetFrameCount = 1;

			ImGui::Text("Add New Spritesheet:");
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("Texture Key##new", newSheetTexture, IM_ARRAYSIZE(newSheetTexture));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputInt("Frame Count##new", &newSheetFrameCount, 1, 5);

			ImGui::SameLine();
			if (ImGui::Button("Add Sheet")) {
				if (std::strlen(newSheetTexture) > 0 && newSheetFrameCount > 0) {
					animData.spritesheets.push_back({ newSheetTexture, newSheetFrameCount });
					newSheetTexture[0] = '\0';
					newSheetFrameCount = 1;
				}
			}

			ImGui::Spacing();
			ImGui::Separator();

			// Save & Reload buttons
			if (ImGui::Button("Save Animation", ImVec2(150.0f, 0.0f))) {
				SaveAnimationFile(animData.name);
				Logger::Get().Info(DebugType::Editor)
					<< "Animation saved and updated: " << animData.name << std::endl;
			}

			ImGui::SameLine();
			if (ImGui::Button("Reload from Disk", ImVec2(150.0f, 0.0f))) {
				ReloadAnimationFromFile(animData.name);
				Logger::Get().Info(DebugType::Editor)
					<< "Animation reloaded from file: " << animData.name << std::endl;
			}

			// Preview info
			ImGui::Spacing();
			ImGui::SeparatorText("Preview");
			ImGui::Text("Total Duration: %.2f seconds", animData.frameCount * animData.frameDuration);
			ImGui::Text("FPS: %.1f", 1.0f / animData.frameDuration);
		}
		else {
			ImGui::TextDisabled("Select an animation from the list to edit");
		}

		ImGui::End();
	}

	void Editor::ReloadAnimationFromFile(const std::string& animationName) {
		using namespace Framework;

		// Find and reload from disk
		int animIndex = -1;
		for (size_t i = 0; i < loadedAnimations_.size(); ++i) {
			if (loadedAnimations_[i].name == animationName) {
				animIndex = static_cast<int>(i);
				break;
			}
		}

		if (animIndex < 0) {
			Logger::Get().Error(DebugType::Editor)
				<< "Animation not found: " << animationName << std::endl;
			return;
		}

		std::string filepath = "Data/AnimationData/" + animationName + ".json";
		rapidjson::Document doc;
		if (FileIOSystem::ReadFile(filepath, doc) && doc.IsObject()) {
			AnimationFileData& animData = loadedAnimations_[animIndex];

			if (doc.HasMember(FileIOKeys::FRAME_COUNT)) {
				animData.frameCount = doc[FileIOKeys::FRAME_COUNT].GetInt();
			}
			if (doc.HasMember(FileIOKeys::FRAME_DURATION)) {
				animData.frameDuration = doc[FileIOKeys::FRAME_DURATION].GetFloat();
			}
			if (doc.HasMember(FileIOKeys::IS_LOOPING)) {
				animData.isLooping = doc[FileIOKeys::IS_LOOPING].GetBool();
			}
			if (doc.HasMember(FileIOKeys::START_FRAME)) {
				animData.startFrame = doc[FileIOKeys::START_FRAME].GetInt();
			}

			// Reload spritesheets
			animData.spritesheets.clear();
			if (doc.HasMember("spritesheets") && doc["spritesheets"].IsArray()) {
				const rapidjson::Value& sheetsJson = doc["spritesheets"];
				for (rapidjson::SizeType i = 0; i < sheetsJson.Size(); ++i) {
					const rapidjson::Value& sheet = sheetsJson[i];
					if (sheet.HasMember("texture") && sheet.HasMember("count")) {
						animData.spritesheets.push_back({
							sheet["texture"].GetString(),
							sheet["count"].GetInt()
						});
					}
				}
			}

			// Refresh animation in ResourceManager
			RefreshAnimationInResourceManager(animationName);

			Logger::Get().Info(DebugType::Editor)
				<< "Reloaded animation from file: " << filepath << " (frameCount: " << animData.frameCount << ")" << std::endl;
		}
		else {
			Logger::Get().Error(DebugType::Editor)
				<< "Failed to reload animation file: " << filepath << std::endl;
		}
	}

}
