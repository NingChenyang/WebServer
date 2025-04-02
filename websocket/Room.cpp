#include "Room.h"

Room::Room(int id, const std::string &name)
    : id_(id), name_(name) {}

void Room::AddMember(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    members_.insert(conn);
}

void Room::RemoveMember(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    members_.erase(conn);
}

void Room::Broadcast(const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto conn : members_)
    {
        if (conn->Connected())
        {
            conn->Send(message);
        }
    }
}