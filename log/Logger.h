#pragma once
#include"LogStream.h"
#include "../tcp/TimeStamp.h"
#include "../tcp/CurrentThread.h"
#include"AsyncLogger.h"
#include <memory>
#include <assert.h>
#include<mutex>
class Logger
{
public:
    static std::string log_file_basename_;
    enum class LogLevel
    {
        DEBUG, // 调试使用
        INFO,  // 信息
        WARN,  // 警告
        ERROR, // 错误
        FATAL, // 致命
        NUM_LOG_LEVELS,
    };
    Logger(const char *filename, int line, LogLevel level, const char *funcname);
    Logger(const char *file, int line);
    Logger(const char *file, int line, LogLevel level);
    Logger(const char *file, int line, bool toAbort);
    ~Logger();
    // 获取全局日志等级，并非当前对象的等级
    static LogLevel GlobalLogLevel();
    LogStream &Stream();
    void DefalutOutput(const char*msg,size_t len);

    static std::string LogFileName() { return log_file_basename_; }

    using OutputFunc = void (*)(const char *msg, int len);
    using FlushFunc = void (*)();
    // 设置输出函数
    static void SetOutput(OutputFunc);
    // 设置冲刷函数
    static void SetFlush(FlushFunc);

private:
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        Impl(LogLevel level, const std::string &file, int line);
        void formatTime();
        void finish();

        TimeStamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;             // 行数 ，即是__LINE__
        std::string filename_; // 写日志消息说在的源文件，使用该函数的文件名(即是 __FILE__ )
    };

    Impl impl_;
    // static std::string log_file_basename_;
};

// 这里是不同的日志等级就会有对应的输出，有些等级会输出多些字段，有些等级的输出字段会少些
//  宏定义，在代码中include该头文件即可使用： LOG_INFO << "输出数据";
#define LOG_TRACE                                            \
    if (Logger::GlobalLogLevel() <= Logger::LogLevel::TRACE) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::TRACE, __func__).Stream()
#define LOG_DEBUG                                            \
    if (Logger::GlobalLogLevel() <= Logger::LogLevel::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG, __func__).Stream()
#define LOG_INFO                                            \
    if (Logger::GlobalLogLevel() <= Logger::LogLevel::INFO) \
    Logger(__FILE__, __LINE__).Stream()

#define LOG_WARN Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).Stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).Stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::LogLevel::FATAL).Stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).Stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).Stream()