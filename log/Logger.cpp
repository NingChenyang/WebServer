#include "Logger.h"

static std::unique_ptr<AsyncLogger> asyncLogger;
static std::once_flag g_once_flag;

// 初始化日志等级
Logger::LogLevel InitLogLevel()
{
    if (getenv("LGG_DEBUF"))
        return Logger::LogLevel::DEBUG;
    else
        return Logger::LogLevel::INFO;
}
// 初始化
// std::string Logger::log_file_basename_ = "../logs/server"; // 修改默认日志路径
void OnceInit()
{
    try
    {
        asyncLogger = std::make_unique<AsyncLogger>(Logger::LogFileName(), 1024 * 1024 * 50);
        asyncLogger->Start();
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "Failed to initialize AsyncLogger: %s\n", e.what());
        exit(1);
    }
}
// 输出到给定的缓冲区中
void AsyncOutput(const char *logline, int len)
{
    if (logline == nullptr || len <= 0)
        return;
    try
    {
        std::call_once(g_once_flag, OnceInit);
        if (asyncLogger)
        {
            asyncLogger->Append(logline, len);
        }
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "AsyncOutput failed: %s\n", e.what());
    }
}
void Logger::DefalutOutput(const char *msg, size_t len)
{
    fwrite(msg, 1, len, stdout);
}
// 默认冲刷函数，冲刷标准输出流
void DefaultFlush()
{
    fflush(stdout);
}
// 全局变量:日志等级
Logger::LogLevel g_LogLevel = InitLogLevel();
// 全局变量：输出函数
Logger::OutputFunc g_output = AsyncOutput;
// 全局变量：冲刷函数
Logger::FlushFunc g_flush = DefaultFlush;
// 获取全局日志等级，并非当前对象的等级
Logger::LogLevel Logger::GlobalLogLevel()
{
    return g_LogLevel;
}

LogStream &Logger::Stream()
{
    return impl_.stream_; // TODO: 在此处插入 return 语句
}

// 当前线程的时间戳
thread_local char t_time[64];
// 当前线程最新日志消息的秒数
thread_local time_t t_lastSecond;

// helper class for known string length at compile time(编译时间)
class T
{
public:
    T(const char *str, unsigned len)
        : str_(str),
          len_(len)
    {
    }
    const char *str_;
    const unsigned len_;
};
inline LogStream &operator<<(LogStream &s, T v)
{
    s.Append(v.str_, v.len_);
    return s;
}

// 日志等级字符串数组
const char *g_loglevel_name[static_cast<int>(Logger::LogLevel::NUM_LOG_LEVELS)] =
    {
        "DEBUG ",
        "INFO  ",
        "WARN  ",
        "ERROR ",
        "FATAL "};

// 多个级别，输出就有对应的输出。
Logger::Logger(const char *FileName, int line, LogLevel level, const char *funcName)
    : impl_(level, FileName, line)
{
    impl_.stream_ << funcName << ' ';
}

Logger::Logger(const char *file, int line)
    : impl_(LogLevel::INFO, file, line)
{
}
Logger::Logger(const char *file, int line, LogLevel level)
    : impl_(level, file, line)
{
}
Logger::Logger(const char *file, int line, bool toAbort)
    : impl_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, file, line)
{
}

Logger::Impl::Impl(LogLevel level, const std::string &file, int line)
    : time_(TimeStamp::Now()), stream_(), level_(level), line_(line), filename_(file)
{
    formatTime();                                                               // 时间输出
    CurrentThread::tid();                                                       // 更新线程
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength()); // 输出线程id
    stream_ << T(g_loglevel_name[static_cast<int>(level_)], 6);                 // 日志等级的字符串是定长的
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t seconds = tv.tv_sec;
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);

    // 格式化年月日时分秒
    char t_time[32];
    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                       tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                       tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);

    // 添加微秒
    char micro[12];
    int microseconds = tv.tv_usec;
    int lenMicro = snprintf(micro, sizeof(micro), ".%06d ", microseconds);

    stream_ << T(t_time, 17) << T(micro, lenMicro);
}

void Logger::Impl::finish()
{
    stream_ << " - " << filename_ << ':' << line_ << '\n';
}

// 设置输出函数
void Logger::SetOutput(OutputFunc out)
{
    g_output = out;
}
// 设置冲刷函数
void Logger::SetFlush(FlushFunc flush)
{
    g_flush = flush;
}
Logger::~Logger()
{
    try
    {
        impl_.finish();
        const LogStream::LogBuffer &buf(Stream().buffer());
        if (g_output && buf.Data() && buf.Length() > 0)
        {
            g_output(buf.Data(), buf.Length());
        }
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "Logger destructor failed: %s\n", e.what());
    }
}
