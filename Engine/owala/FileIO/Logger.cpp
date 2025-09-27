#include "Logger.h"

namespace Framework {
    const std::string Logger::LOGS_FILEPATH = "Logs/", Logger::LOGS_FILEEXT = "_crash.log";

    const void Logger::InitSymbols() {
        // SYMOPT_DEFERRED_LOADS: load symbols only when its needed
        // SYMOPT_LOAD_LINES: load file and line number information in addition to function names
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

        if (!SymInitialize(GetCurrentProcess(),
            "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols",
            TRUE)) {
            Get().Error(DebugType::Logger) << "Failed to initialize symbols. Error: " << GetLastError() << std::endl;
        }
    }
    
    // called when GameException is thrown. uses stack trace captured at the point where GameException was thrown
    void Logger::LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace) {
        Get().Error(type) << message << std::endl; // log to console for debugging

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Get().Error(DebugType::Logger) << "Unable to open " << logPath << std::endl;
        } else {
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(stack_trace, stderr); // log to console for debugging

            // write to log file
            Get().Info(DebugType::Logger) << "Writing to " << logPath << std::endl;
            logFile << GetDebugType(type) << message << '\n' << std::endl;
            p.print(stack_trace, logFile);
        }
    }

    // called when other exceptions are thrown. captures stack trace at this point
    void Logger::LogCrash(const DebugType type, const std::string& message) {
        Get().Error(type) << message << std::endl; // log to console for debugging

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Get().Error(DebugType::Logger) << "Unable to open " << logPath << std::endl;
        }
        else {
            backward::StackTrace st; st.load_here(32);
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(st, stderr); // log to console for debugging

            // write to log file
            Get().Info(DebugType::Logger) << "Writing to " << logPath << std::endl;
            logFile << GetDebugType(type) << message << '\n' << std::endl;
            p.print(st, logFile);
        }
    }

    std::ostream& Logger::Info(DebugType type) {
        std::cout << GetDebugType(type) << ' ';
        return std::cout;
    }

    std::ostream& Logger::Error(DebugType type) {
        std::cerr << "[ERROR] " << GetDebugType(type) << ' ';
        return std::cerr;
    }
}
