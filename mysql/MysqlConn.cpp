#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
    conn_ = mysql_init(nullptr);
    if(!conn_)
    {
        // LOG_WARN << "mysql_init failed";
    }
    mysql_set_character_set(conn_, "utf8mb4");
}

MysqlConn::~MysqlConn()
{
    if(conn_)
    {
        mysql_close(conn_);
    }
    FreeResult();
}

bool MysqlConn::Connect(const string &host, const string &user, const string &passwd, const string &db, unsigned int port)
{
    if(!mysql_real_connect(conn_, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0))
    {
        // LOG_WARN << "mysql_real_connect failed";
        return false;
    }
    return true;
}

bool MysqlConn::Update(const string &sql)
{
    if(mysql_query(conn_, sql.c_str()))
    {
        // LOG_WARN<< "mysql_query failed" ;
        return false;
    }
    return true;
}

bool MysqlConn::Query(const string &sql)
{
    FreeResult();
    if(mysql_query(conn_, sql.c_str())!=0)
    {
        // LOG_WARN << "mysql_query failed";
        return false;
    }
    res_ = mysql_store_result(conn_);
    return true;
}

bool MysqlConn::Next()
{
    if(res_)
    {
        row_ = mysql_fetch_row(res_);
        if(row_)
        {
            return true;
        }
    }
    return false;
}

string MysqlConn::Value(int index)
{
    if(!row_)
    {
        return string();
    }
    int num_fields = mysql_num_fields(res_);
    if(num_fields==0|| index>= num_fields||index<0)
    {
        return string();
    }

    return string(row_[index], mysql_fetch_lengths(res_)[index]);
}

bool MysqlConn::Transaction()
{

    return !mysql_autocommit(conn_, 0);
}

bool MysqlConn::Commit()
{
    return !mysql_commit(conn_);
}

bool MysqlConn::Rollback()
{
    return !mysql_rollback(conn_);
}

void MysqlConn::RefreshAliveTime()
{
    alive_time_ = steady_clock::now();
}

long long MysqlConn::GetAliveTime()
{
    nanoseconds ns = steady_clock::now() - alive_time_;
    milliseconds ms = duration_cast<milliseconds>(ns);
    return ms.count();
}

void MysqlConn::FreeResult()
{
    if(res_)
    {
        mysql_free_result(res_);
        res_ = nullptr;
    }
}
