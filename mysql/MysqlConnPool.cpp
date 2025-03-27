#include "MysqlConnPool.h"

MysqlConnPool::MysqlConnPool()
{

    if (!ParseJsonConfig())
    {
        return;
    }
        for (int i = 0; i < min_conn_; i++)
        {
            AddConn();
        }
    thread produce_thread(&MysqlConnPool::ProduceConn, this);
    thread destroy_thread(&MysqlConnPool::DestroyConn, this);
    produce_thread.detach();
    destroy_thread.detach();
}

bool MysqlConnPool::ParseJsonConfig()
{
    fstream file("mysql_config.json");
    if (!file.is_open())
    {
        return false;
    }
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(file, root))
    {
        return false;
    }
    if (root.isObject())
    {
        host_ = root["host"].asString();
        user_ = root["user"].asString();
        passwd_ = root["passwd"].asString();
        db_ = root["db"].asString();
        port_ = root["port"].asUInt();
        max_conn_ = root["max_conn"].asUInt();
        min_conn_ = root["min_conn"].asUInt();
        time_out_ = root["time_out"].asInt();
        max_idle_time_ = root["max_idle_time"].asInt();
        file.close();
        return true;
    }

    file.close();
    return false;
}

void MysqlConnPool::ProduceConn()
{
    while (!stop_)
    {
        unique_lock<mutex> lock(mutexQ_);
        while (mysql_connQ_.size() >= min_conn_)
        {
            condQ_.wait(lock);
            if (stop_)
            {
                return;
            }
            
        }
        AddConn();
        condQ_.notify_all();
    }
}

void MysqlConnPool::DestroyConn()
{
    while (true)
    {
        this_thread::sleep_for(chrono::milliseconds(500));
        lock_guard<mutex> lock(mutexQ_);
        while (mysql_connQ_.size() > min_conn_)
        {
            if (stop_)
            {
                break;
            }
            
            MysqlConn *conn = mysql_connQ_.front();
            if (conn->GetAliveTime() >= max_idle_time_)
            {
                mysql_connQ_.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}

void MysqlConnPool::AddConn()
{
    MysqlConn *conn = new MysqlConn();
    if (!conn->Connect(host_, user_, passwd_, db_, port_))
    {
        delete conn;
        return;
    }
    conn->RefreshAliveTime();
    mysql_connQ_.push(conn);
}

MysqlConnPool *MysqlConnPool::GetInstance()
{
    static MysqlConnPool mysql_conn_pool;
    return &mysql_conn_pool;
}

std::shared_ptr<MysqlConn> MysqlConnPool::GetConn()
{
    unique_lock<mutex> lock(mutexQ_);
    while (mysql_connQ_.empty())
    {
        if (cv_status::timeout == condQ_.wait_for(lock, chrono::milliseconds(time_out_)))
        {
            if (mysql_connQ_.empty())
            {
                // 先进行循环等待
                continue;
            }
        }
    }
    shared_ptr<MysqlConn> conn_ptr(mysql_connQ_.front(), [this](MysqlConn *conn)
                                   {
        unique_lock<mutex> lock(mutexQ_);
        conn->RefreshAliveTime();
        mysql_connQ_.push(conn);
        //由于上面进行循环等待连接，需要唤醒
        // condQ_.notify_one(); 
    });
    mysql_connQ_.pop();
    // 唤醒生产者线程
    condQ_.notify_all();
    return conn_ptr;
}

MysqlConnPool::~MysqlConnPool()
{
    while (!mysql_connQ_.empty())
    {
        MysqlConn *conn = mysql_connQ_.front();
        mysql_connQ_.pop();
        delete conn;
    }
}

void MysqlConnPool::ShutDown()
{
    stop_ = true;
    condQ_.notify_all();
    while (!mysql_connQ_.empty())
    {
        MysqlConn *conn = mysql_connQ_.front();
        mysql_connQ_.pop();
        delete conn;
    }
}
