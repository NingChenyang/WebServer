#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include "../tcp/util.h"
#include "../tcp/Connection.h"
#include "../tcp/Buffer.h"
#include<mutex>
class Room
{
public:
    Room(int id, const std::string &name);
    ~Room(){};

    void AddMember(ConnectionPtr conn);
    void RemoveMember(ConnectionPtr conn);
    void Broadcast(const std::string &message);
    
    int GetId() const { return id_; }
    std::string GetName() const { return name_; }

private:
    int id_;
    std::string name_;
    std::mutex mutex_;
    std::unordered_set<ConnectionPtr> members_; // 使用无序集合存储连接指针
};
