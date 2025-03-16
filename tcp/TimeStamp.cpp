#include "TimeStamp.h"

TimeStamp::TimeStamp()
{
    secsinceepoch_ = time(0);
}

TimeStamp::TimeStamp(int64_t secsinceepoch) : secsinceepoch_(secsinceepoch)
{
}

TimeStamp TimeStamp::Now()
{
    return TimeStamp();
}

std::string TimeStamp::NowToString()
{
    return TimeStamp().TimeToString();
}

time_t TimeStamp::TimeToInt()
{
    return secsinceepoch_;
}

std::string TimeStamp::TimeToString()
{
    char time[32] = { 0 };
    tm* tm = localtime(&secsinceepoch_);
    snprintf(time, 32, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return std::string(time);
}

TimeStamp::~TimeStamp()
{
}