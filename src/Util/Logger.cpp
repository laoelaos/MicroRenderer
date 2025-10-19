//
// Created by laoe on 2025/10/18.
//

#include "Logger.h"
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace {
    const char* LevelToString(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::Trace:    return "TRACE";
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warn:     return "WARN";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    std::string NowTimestamp() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm_buf{};
    #if defined(_WIN32)
        localtime_s(&tm_buf, &t);
    #else
        localtime_r(&t, &tm_buf);
    #endif
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << '.'
            << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
}

struct Logger::Impl {
    std::mutex mtx;
    std::ofstream file;
    bool console = true;
    LogLevel minLevel = LogLevel::Info;

    void writeLine(LogLevel lvl, const std::string& msg) {
        if (lvl < minLevel) return;

        const std::string line = '[' + NowTimestamp() + "] [" + LevelToString(lvl) + "] " + msg + '\n';
        std::lock_guard<std::mutex> lk(mtx);

        if (file.is_open()) {
            file << line;
            file.flush();
        }

        if (console) {
            if (lvl >= LogLevel::Error) {
                std::cerr << line;
            } else {
                std::cout << line;
            }
        }
    }
};

Logger& Logger::Get() {
    static Logger inst;
    return inst;
}

Logger::Logger() : pimpl(new Impl) {}
Logger::~Logger() { delete pimpl; }

bool Logger::setFile(const std::string &filepath, bool append) {
    std::lock_guard<std::mutex> lk(pimpl->mtx);
    if (pimpl->file.is_open()) pimpl->file.close();
    std::ios_base::openmode mode = std::ios::out;
    if (append) mode |= std::ios::app; else mode |= std::ios::trunc;
    pimpl->file.open(filepath, mode);
    return pimpl->file.is_open();
}

void Logger::enableConsole(bool enable) {
    std::lock_guard<std::mutex> lk(pimpl->mtx);
    pimpl->console = enable;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lk(pimpl->mtx);
    pimpl->minLevel = level;
}

void Logger::log(LogLevel level, const std::string &message) {
    pimpl->writeLine(level, message);
}

void Logger::trace(const std::string &msg)   { log(LogLevel::Trace, msg); }
void Logger::debug(const std::string &msg)   { log(LogLevel::Debug, msg); }
void Logger::info(const std::string &msg)    { log(LogLevel::Info, msg); }
void Logger::warn(const std::string &msg)    { log(LogLevel::Warn, msg); }
void Logger::error(const std::string &msg)   { log(LogLevel::Error, msg); }
void Logger::critical(const std::string &msg){ log(LogLevel::Critical, msg); }

