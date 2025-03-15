#pragma once
#include <string>
#include <cstring>
#include "FileUtil.h"
#include <memory>
#include<ctime>
#include<unistd.h>
#include"../util.h"
class LogFile
{
public:
    // LogFile(const std::string &filename_, int flush_everyN = 1024);
    LogFile(const std::string &basename, off_t roll_size, int flush_interval_second = 3, int flush_everyN = 1024);
    ~LogFile();
    void Append(const char *str, int len);

    void Flush();
    // 滚动日志
    bool RollFile();

private:
    std::string GetLogFileName(const std::string &basename, time_t *now);
    // const std::string filename_;
    const std::string basename_;
    const off_t rollsize_;
    // 冲刷的时间间隔
    const int flush_interval_second_;
    // 冲刷的次数限制
    const int flush_everyN_;
    int count_;
    std::unique_ptr<AppendFile> file_;
    // 日志文件开始写的时间
    time_t start_of_period_;
    // 最新一次的滚动时间
    time_t last_roll_;
    // 一天中的秒数
    const static int kRollPerSeconds_ = 60 * 60 * 24;
};
