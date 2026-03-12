/*****************************************************************//**
* \file   pch.h
*
* \primary_author
*      Ng Jing Wen
* \secondary_author
*
* \brief    Precompiled header file for the game project. This file includes commonly used standard library headers,
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once

// =============================================================================
// Standard Library Headers (Most frequently used, stable)
// =============================================================================
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <cstdio>
#include <map>
#include <functional>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <array>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <optional>
#include <variant>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <exception>
#include <utility>
#include <tuple>
#include <filesystem>
#include <random>
#include <cctype>

// =============================================================================
// Windows Platform Headers
// =============================================================================
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shlobj.h>
#endif

// =============================================================================
// Third-Party Library Headers (Stable, rarely change)
// =============================================================================

// RapidJSON
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/istreamwrapper.h>

// Graphics Libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui (if used in Editor)
#ifdef BUILD_EDITOR
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#endif

// =============================================================================
// Forward Declarations (Reduce header dependencies)
// =============================================================================
namespace Framework {
    class EntityStorage;
    class EntityManager;
    class GameManager;
    class SceneManager;
    class ResourceManager;
    class CameraSystem;
    class PhysicsSystem;
    class AudioSystem;
    class LayerManager;
}

namespace Editor {
    class Editor;
}

// DEFINES

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif