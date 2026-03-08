/*****************************************************************//**
 * \file   Logger.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief
 *      This file contains the core components for engine-wide logging and error reporting.
 *      It provides a `GameException` struct that automatically captures a stack trace
 *      for easier debugging, and a singleton `Logger` class. The logger can output
 *      categorized messages to the console and create timestamped crash log files.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include "backward.hpp"
#include <windows.h>

const enum class DebugType {
    Unknown,
    WindowsSystem,
    InputSystem,
    PhysicsSystem,
    AudioSystem,
    SceneManager,
    EntityManager,
    TransformationSystem,
    PlayerSystem,
    GraphicsSystem,
    SpriteRenderSystem,
    TextRenderSystem,
    ResourceManager,
    GameManager,
    FileIOSystem,
    Logger,
    Editor,
    Font,
    EntitySeralizer,
    AISystem,
    LevelResetSystem,
    GUISystem,
    VFXSystem,
	CameraSystem,
	FireBaseManager
};

const std::string DebugTypeLabels[] = {
    "Unknown Exception",
    "WindowsSystem",
    "InputSystem",
    "PhysicsSystem",
	"AudioSystem",
    "SceneManager",
    "EntityManager",
    "TransformationSystem",
    "PlayerSystem",
    "GraphicsSystem",
    "SpriteRenderSystem",
    "TextRenderSystem",
    "ResourceManager",
    "GameManager",
    "FileIOSystem",
    "Logger",
    "Editor",
    "Font",
    "EntitySeralizer",
    "AISystem",
    "LevelResetSystem",
    "GUISystem",
    "VFXSystem",
	"CameraSystem"
};

namespace Framework {

    // throw GameException with DebugType and a msg instead of runtime_error
    const struct GameException : public std::exception {
        DebugType type;
        std::string msg;
        backward::StackTrace stack_trace;

        GameException(DebugType c, std::string m) : type(c), msg(std::move(m)) {
            stack_trace.load_here(); // capture the stack at the point when it was thrown
        }

        const char* what() const noexcept override {
            return msg.c_str();
        }
    };

	class Logger {
    public:
        // need this to resolve the symbols in stack trace
        static const void InitSymbols();

        // make logger singleton
        static Logger& Get() {
            static Logger instance;
            return instance;
        }

        // for general debugging statements
        static std::ostream& Info(DebugType type);
        // for error debugging
        static std::ostream& Error(DebugType type);
        
        // functions to log crashes to a log file
        // for GameExceptions thrown
        static void LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace);
        // for other exceptions caught
        static void LogCrash(const DebugType type, const std::string& message);

    private:
        Logger() = default;

        static const std::string LOGS_FILEPATH, LOGS_FILEEXT;

        static const std::string GetDebugType(DebugType type) {
            return '[' + DebugTypeLabels[(int)type] + "] ";
        }

        static const std::string GetCrashLogFilename();
	};
} // namespace Framework
