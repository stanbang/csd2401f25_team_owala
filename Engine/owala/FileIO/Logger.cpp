/*****************************************************************//**
 * \file   Logger.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include "Logger.h"

namespace Framework {
    const std::string Logger::LOGS_FILEPATH = "Logs/", Logger::LOGS_FILEEXT = "_crash.log";

    const void Logger::InitSymbols() {
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        if (!SymInitialize(GetCurrentProcess(),
            "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols",
            TRUE)) {
            std::cerr << "Failed to initialize symbols. Error: " << GetLastError() << "\n";
        }
    }
    
    // called when GameException is thrown. uses stack trace captured at the point where GameException was thrown
    void Logger::LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace) {
        Info(type, message); // log to console for debugging

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Info(DebugType::Logger, "Unable to open " + logPath);
        } else {
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(stack_trace, stderr); // log to console for debugging

            // write to log file
            Info(DebugType::Logger, "Writing to " + logPath);
            logFile << GetErrorMsg(type, message) << '\n' << std::endl;
            p.print(stack_trace, logFile);
        }
    }

    // called when other exceptions are thrown. captures stack trace at this point
    void Logger::LogCrash(const DebugType type, const std::string& message) {
        Info(type, message); // log to console for debugging

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Info(DebugType::Logger, "Unable to open " + logPath);
        }
        else {
            backward::StackTrace st; st.load_here(32);
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(st, stderr); // log to console for debugging

            // write to log file
            Info(DebugType::Logger, "Writing to " + logPath);
            logFile << GetErrorMsg(type, message) << '\n' << std::endl;
            p.print(st, logFile);
        }
    }

    void Logger::Info(DebugType type, const std::string message) {
        std::cout << GetErrorMsg(type, message) << std::endl;
    }

}
