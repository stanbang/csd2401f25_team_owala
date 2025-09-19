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
    void Logger::logCrash(const DebugType type, const std::string& message) {
        debugLog(type, message);

        if (!filesystem::is_directory(LOGS_FILEPATH)) {
            debugLog(DebugType::Logger, LOGS_FILEPATH + " directory not found, creating directory");
            filesystem::create_directory(LOGS_FILEPATH);
        }

        string logPath = getCrashLogFilename();
        ofstream log(logPath, ios::app);

        if (!log) {
            debugLog(DebugType::Logger, "Unable to open " + logPath);
        } else {
            debugLog(DebugType::Logger, "Writing to " + logPath);
            log << getErrorMsg(type, message) << endl;
        }
    }

    void Logger::debugLog(DebugType type, const string message) {
        cout << getErrorMsg(type, message) << endl;
    }

}
