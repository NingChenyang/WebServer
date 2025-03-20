#include "LogFile.h"
LogFile::LogFile(const std::string &basename, off_t roll_size, int flush_interval_second, int flush_everyN)
    : basename_(basename), rollsize_(roll_size), flush_interval_second_(flush_interval_second), flush_everyN_(flush_everyN), count_(0), start_of_period_(0), last_roll_(0), file_(std::make_unique<AppendFile>(basename))
{
    RollFile();
}

LogFile::~LogFile()
{
}

void LogFile::Append(const char *str, int len)
{
    file_->Append(str, len);
    if (file_->WrittenBytes() >= rollsize_)
    {
        RollFile();
    }
    else
    {
        ++count_;
        if (count_ >= flush_everyN_)
        {
            time_t now=time(nullptr);
            time_t this_period = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (this_period!=start_of_period_)
            {
                RollFile();
            }
            else
            {
                if (now - last_roll_ > flush_interval_second_)
                {
                    last_roll_ = now;
                    file_->Flush();
                }
            }
            
            count_ = 0;
            
        }
    }
    

    
}

void LogFile::Flush()
{
    file_->Flush();
}

bool LogFile::RollFile()
{
    time_t now = time(nullptr);
    if (now>last_roll_)
    {
        last_roll_=now;
        start_of_period_=now/kRollPerSeconds_*kRollPerSeconds_;
        std::string filename=GetLogFileName(basename_,&now);
        last_roll_=now;
        file_.reset(new AppendFile(filename));
        file_->Flush();
        count_=0;
        return true;
    }
    return false;
}

std::string LogFile::GetLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename=basename;

    char timebuf[32];
    struct tm tm;
    memset(&tm,0,sizeof(tm));
    //_r线程更安全
    localtime_r(now,&tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);

    filename+=timebuf;
    filename+=ProcessInfo::hostname();

    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), ".%d.log", ProcessInfo::pid());
    filename+=pidbuf;
    return filename;
}
