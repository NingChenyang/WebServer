#include "Room.h"
std::string GetCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
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

void Room::Broadcast(const Json::Value mesg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto conn : members_)
    {
        if (conn->Connected())
        {
            WebSocketPacket resp;
            resp.SetOpcode(WebSocketOpcode::kTextFrame);
            Buffer frame;
            Buffer send_buf;
            Json::StreamWriterBuilder writer;
            writer["emitUTF8"] = true;
            send_buf.Append(Json::writeString(writer, mesg));
            // std::cout << send_buf.Peek() << std::endl;
            resp.EncodeFrame(&frame, &send_buf);

            conn->Send(&frame);
        }
    }
}