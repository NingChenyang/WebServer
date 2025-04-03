#pragma once
#include "MysqlConn.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <fstream>
#include "../jsoncpp/json/json.h"
#include <atomic>
class MysqlConnPool
{
public:
    // 饿汉模式直接返回实例
    static MysqlConnPool *GetInstance()
    {
        return &instance_;
    }

    // 删除拷贝构造函数和赋值运算符
    MysqlConnPool(const MysqlConnPool &) = delete;
    MysqlConnPool &operator=(const MysqlConnPool &) = delete;
    // 获得一个数据库连接
    std::shared_ptr<MysqlConn> GetConn();
    ~MysqlConnPool();
    void ShutDown();

private:
    // 单例模式
    //  静态实例
    static MysqlConnPool instance_;

    // 构造函数私有化
    MysqlConnPool();
    // 解析json配置文件
    bool ParseJsonConfig();
    // 产生数据库连接
    void ProduceConn();
    // 销毁数据库连接
    void DestroyConn();
    // 添加数据库连接
    void AddConn();

    // 基本信息
    string host_;
    string user_;
    string passwd_;
    string db_;
    unsigned short port_;

    // 关于连接池的信息

    // 存放数据库连接的队列
    std::queue<MysqlConn *> mysql_connQ_;
    // 最大连接数
    unsigned int max_conn_;
    // 最小连接数
    unsigned int min_conn_;
    // 连接超时时长
    int time_out_;
    // 最大空闲时长
    int max_idle_time_;
    // 互斥锁
    std::mutex mutexQ_;
    // 条件变量
    std::condition_variable condQ_;
    std::atomic<bool> stop_;
};
