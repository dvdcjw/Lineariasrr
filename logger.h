#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    static Logger& GetInstance();
    void Log(const std::string& message);
    void LogFormat(const char* format, ...);

private:
    Logger();
    ~Logger();
    
    std::ofstream logFile;
    std::mutex logMutex;
};

#define LOG_MSG(msg) Logger::GetInstance().Log(msg)
#define LOG_FMT(fmt, ...) Logger::GetInstance().LogFormat(fmt, __VA_ARGS__)

#endif // LOGGER_H
