//
// Created by laoe on 2025/10/18.
//

#ifndef MICRORENDERER_LOGGER_H
#define MICRORENDERER_LOGGER_H

#include <string>
#include <format>

#define LOGC(...) Logger::Get().criticalf(__VA_ARGS__)
#define LOGE(...) Logger::Get().errorf(__VA_ARGS__)
#define LOGW(...) Logger::Get().warnf(__VA_ARGS__)
#define LOGI(...) Logger::Get().infof(__VA_ARGS__)
#define LOGD(...) Logger::Get().debugf(__VA_ARGS__)
#define LOGT(...) Logger::Get().tracef(__VA_ARGS__)

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

    template<typename... Args>
    void logf(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        try {
            std::string msg = std::format(fmt, std::forward<Args>(args)...);
            log(level, msg);
        } catch (const std::format_error& e) {
            log(LogLevel::Error, std::string("Format error: ") + e.what());
        }
    }

    void trace(const std::string& msg);    void debug(const std::string& msg);
    void info(const std::string& msg);     void warn(const std::string& msg);
    void error(const std::string& msg);    void critical(const std::string& msg);

    template<typename... Args>
    void tracef(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Trace, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void debugf(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Debug, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void infof(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Info, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void warnf(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Warn, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void errorf(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Error, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void criticalf(std::format_string<Args...> fmt, Args&&... args) { logf(LogLevel::Critical, fmt, std::forward<Args>(args)...); }
};

#endif //MICRORENDERER_LOGGER_H

