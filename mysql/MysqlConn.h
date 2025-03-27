#pragma once
#include <mysql.h>
#include <iostream>
#include <chrono>
#include"../log/Logger.h"
using namespace std;
using namespace std::chrono;
class MysqlConn
{
public:
    MysqlConn();
    ~MysqlConn();
    bool Connect(const string &host, const string &user, const string &passwd, const string &db, unsigned int port=3306);
    //更新操作
    bool Update(const string &sql);
    //查询
    bool Query(const string &sql);
    //获取结果集
    bool Next();
    //获取结果集中的数据
    string Value(int index);
    //事务操作,设置事务是否自动提交
    bool Transaction();
    //事务提交
    bool Commit();
    //事务回滚
    bool Rollback();
    //刷新起始的时间
    void RefreshAliveTime();
    //计算连接存活的的总时长
    long long GetAliveTime();

private:
    void FreeResult();
    MYSQL *conn_=nullptr;
    MYSQL_RES *res_=nullptr;
    MYSQL_ROW row_=nullptr;
    // MYSQL_FIELD *field_ = nullptr;
    steady_clock::time_point alive_time_;
};
