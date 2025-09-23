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

    // called when GameException is thrown or any other exception is thrown
    void Logger::LogCrash(const DebugType type, const std::string& message) {
        Info(type, message); // log to console for debugging

        std::string logPath = GetCrashLogFilename();
        std::ofstream log(logPath, std::ios::app);

        if (!log) {
            Info(DebugType::Logger, "Unable to open " + logPath);
        } else {
            Info(DebugType::Logger, "Writing to " + logPath);
            log << GetErrorMsg(type, message) << std::endl;

            backward::StackTrace st; st.load_here(32);
            backward::Printer p;

            p.object = true;
            p.color_mode = backward::ColorMode::always;
            p.address = true;
            p.print(st, stderr);
            p.print(st, log);

         /*   backward::TraceResolver tr; tr.load_stacktrace(st);
            for (size_t i = 0; i < st.size(); ++i) {
                backward::ResolvedTrace trace = tr.resolve(st[i]);
                std::cout << "#" << i
                    << " " << trace.object_filename
                    << " " << trace.object_function
                    << " [" << trace.addr << "]"
                    << std::endl;
            }*/
        }
    }

    void Logger::Info(DebugType type, const std::string message) {
        std::cout << GetErrorMsg(type, message) << std::endl;
    }

}
