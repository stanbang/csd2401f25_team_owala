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

// for formatting logging msgs to be passed into Logger
#include <format>
#include <string>

using namespace std;

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

const string DebugTypeLabels[] = {
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

    const struct GameException : public exception {
        DebugType type;
        string msg;

        GameException(DebugType c, string m) : type(c), msg(std::move(m)) {}

        const char* what() const noexcept override {
            return msg.c_str();
        }
    };

	class Logger {
		static const string LOGS_FILEPATH, LOGS_FILEEXT;

        static const string GetErrorMsg(DebugType type, const string message) {
            return '[' + DebugTypeLabels[(int)type] + "] " + message;
        }

        static const string GetCrashLogFilename() {
            ostringstream oss;
            time_t t = std::time(nullptr);
            tm tm = *std::localtime(&t);

            oss << LOGS_FILEPATH
                << put_time(&tm, "%Y-%m-%d_%H-%M-%S")
                << LOGS_FILEEXT;
            return oss.str();
        }

	public:
        void LogCrash(const DebugType type, const std::string& message);
        void DebugLog(DebugType type, const string message);
	};
} // namespace Framework
