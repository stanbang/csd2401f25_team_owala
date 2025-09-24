/*****************************************************************//**
 * \file   Logger.h
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/
#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "backward.hpp"
#include <windows.h>

// for formatting logging msgs to be passed into Logger
#include <format>
#include <string>

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
    GameSystem,
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
    "GameSystem",
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
    private:
		static const std::string LOGS_FILEPATH, LOGS_FILEEXT;

        static const std::string GetErrorMsg(DebugType type, const std::string message) {
            return '[' + DebugTypeLabels[(int)type] + "] " + message;
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

    public:
        // need this to resolve the symbols in stack trace
        static const void InitSymbols();
        // for GameExceptions thrown
        void LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace);
        // for other exceptions caught
        void LogCrash(const DebugType type, const std::string& message);
        // for general debugging statements
        void Info(DebugType type, const std::string message);
	};
} // namespace Framework
