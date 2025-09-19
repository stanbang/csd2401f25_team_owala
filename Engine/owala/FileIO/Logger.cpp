/*****************************************************************//**
 * \file   Logger.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include "Logger.h"

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>

using namespace std;

namespace Framework {

    const string Logger::LOGS_FILEPATH = "Logs/", Logger::LOGS_FILEEXT = "_crash.log";

    // called when GameException is thrown or any other exception is thrown
    void Logger::LogCrash(const DebugType type, const std::string& message) {
        DebugLog(type, message);

        if (!filesystem::is_directory(LOGS_FILEPATH)) {
            DebugLog(DebugType::Logger, LOGS_FILEPATH + " directory not found, creating directory");
            filesystem::create_directory(LOGS_FILEPATH);
        }

        string logPath = GetCrashLogFilename();
        ofstream log(logPath, ios::app);

        if (!log) {
            DebugLog(DebugType::Logger, "Unable to open " + logPath);
        } else {
            DebugLog(DebugType::Logger, "Writing to " + logPath);
            log << GetErrorMsg(type, message) << endl;
        }
    }

    void Logger::DebugLog(DebugType type, const string message) {
        cout << GetErrorMsg(type, message) << endl;
    }

}
