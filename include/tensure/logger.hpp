#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>

enum class LogLevel { INFO, WARN, ERROR, DEBUG };

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mtx_);
        file_.open(filename, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    void setConsoleOnly(bool enable) {
        console_only_ = enable;
    }

    void log(LogLevel level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::string prefix = "[" + timestamp() + "] " + levelToString(level) + ": ";
        std::string full = prefix + msg + "\n";

        // Always print to console
        std::cerr << full;

        // Optionally write to file
        if (file_.is_open() && !console_only_) {
            file_ << full;
            file_.flush();
        }
    }

private:
    std::ofstream file_;
    bool console_only_ = false;
    std::mutex mtx_;

    Logger() = default;
    ~Logger() {
        if (file_.is_open()) file_.close();
    }

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    std::string levelToString(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::INFO: return "[INFO]";
            case LogLevel::WARN: return "[WARN]";
            case LogLevel::ERROR: return "[ERROR]";
            case LogLevel::DEBUG: return "[DEBUG]";
            default: return "[UNK]";
        }
    }
};

// Convenience macros
#define LOG_INFO(msg)  Logger::instance().log(LogLevel::INFO, msg)
#define LOG_WARN(msg)  Logger::instance().log(LogLevel::WARN, msg)
#define LOG_ERROR(msg) Logger::instance().log(LogLevel::ERROR, msg)
#define LOG_DEBUG(msg) Logger::instance().log(LogLevel::DEBUG, msg)
