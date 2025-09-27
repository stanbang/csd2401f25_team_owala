/*****************************************************************//**
 * \file   Logger.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief
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
    SceneManager,
    RotationSystem,
    PlayerSystem,
    GraphicsSystem,
    ResourceManager,
    GameManager,
    FileIOSystem,
    Logger
};

const std::string DebugTypeLabels[] = {
    "Unknown Exception",
    "WindowsSystem",
    "InputSystem",
    "PhysicsSystem",
    "SceneManager",
    "RotationSystem",
    "PlayerSystem",
    "GraphicsSystem",
    "ResourceManager",
    "GameManager",
    "FileIOSystem",
    "Logger"
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

        static const std::string GetCrashLogFilename() {
            std::ostringstream oss;
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);

            oss << LOGS_FILEPATH
                << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
                << LOGS_FILEEXT;
            return oss.str();
        }
	};
} // namespace Framework
