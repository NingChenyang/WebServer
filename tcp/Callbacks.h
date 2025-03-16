#pragma once

#include <functional>
#include <memory>
#include<string>
class Buffer;
class Connection;
class InetAddress;
using ConnectionPtr = std::shared_ptr<Connection>;
using TimerCallback = std::function<void()>;
using RemoveLoopConnCallback=std::function<void(const ConnectionPtr& conn)>;
using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& peerAddr)>;

using CloseCallback = std::function<void(const ConnectionPtr&)>;


//用户自定
using OnSentCallback = std::function<void( ConnectionPtr)>;
using OnClosedCallback = std::function<void( ConnectionPtr)>;
using OnConnectedCallback = std::function<void( ConnectionPtr)>;
// the data has been read to (buf, len)
using OnMessageCallback = std::function<void(ConnectionPtr, std::string&)>;
