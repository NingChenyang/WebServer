#pragma once
#include<iostream>
#include<string>
class TimeStamp
{
public:
    TimeStamp();
    TimeStamp(int64_t secsinceepoch);
    static TimeStamp Now();
    static std::string NowToString();
    time_t TimeToInt();
    std::string TimeToString();
    ~TimeStamp();
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    time_t secsinceepoch_;
    
};
