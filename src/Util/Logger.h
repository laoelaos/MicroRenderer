//
// Created by laoe on 2025/10/18.
//

#ifndef MICRORENDERER_LOGGER_H
#define MICRORENDERER_LOGGER_H

#include <string>

#define LOGE(...) Logger::Get().errorf(__VA_ARGS__)
#define LOGW(...) Logger::Get().warnf(__VA_ARGS__)
#define LOGI(...) Logger::Get().infof(__VA_ARGS__)
#define LOGD(...) Logger::Get().debugf(__VA_ARGS__)

enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
};

class Logger {
    struct Impl;
    Impl* pimpl; // PIMPL to keep header lightweight

    Logger();
    ~Logger();
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& Get();

    // 设置日志输出文件。如果文件已打开，将关闭并重新打开。
    // 如果append为false，文件将被截断。
    bool setFile(const std::string& filepath, bool append = true);
    // 设置是否要在控制台输出日志
    void enableConsole(bool enable);
    // 设置最小日志级别, 低于该级别的日志将被忽略
    void setLevel(LogLevel level);

    void log(LogLevel level, const std::string& message);
    void logf(LogLevel level, const char* fmt, ...);
    void trace(const std::string& msg);    void debug(const std::string& msg);
    void info(const std::string& msg);     void warn(const std::string& msg);
    void error(const std::string& msg);    void critical(const std::string& msg);

    void tracef(const char* fmt, ...);     void debugf(const char* fmt, ...);
    void infof(const char* fmt, ...);      void warnf(const char* fmt, ...);
    void errorf(const char* fmt, ...);     void criticalf(const char* fmt, ...);
};

#endif //MICRORENDERER_LOGGER_H