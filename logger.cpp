#include "logger.h"
#include <windows.h>
#include <iostream>
#include <cstdarg>
#include <vector>
#include <ctime>

Logger::Logger() {
    char dllPath[MAX_PATH];
    HMODULE hMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&Logger::GetInstance, &hMod);
    
    std::string logFilePath = "dinput8_lut.log";
    if (hMod) {
        GetModuleFileNameA(hMod, dllPath, MAX_PATH);
        std::string pathStr(dllPath);
        size_t lastSlash = pathStr.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            logFilePath = pathStr.substr(0, lastSlash + 1) + "dinput8_lut.log";
        }
    }
    
    logFile.open(logFilePath, std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logFile << "[INIT] Logger started. Log file: " << logFilePath << std::endl;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile << "[SHUTDOWN] Logger closed." << std::endl;
        logFile.close();
    }
}

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!logFile.is_open()) return;

    time_t now = time(nullptr);
    struct tm tstruct;
    localtime_s(&tstruct, &now);
    char timeBuf[80];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tstruct);

    logFile << "[" << timeBuf << "] " << message << std::endl;
}

void Logger::LogFormat(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Estimate size
    int size = vsnprintf(nullptr, 0, format, args) + 1;
    va_end(args);

    if (size <= 0) return;

    std::vector<char> buf(size);
    va_start(args, format);
    vsnprintf(buf.data(), size, format, args);
    va_end(args);

    Log(std::string(buf.data()));
}
