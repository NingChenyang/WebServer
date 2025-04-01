
#pragma once
#include "sha1.h"
#include "WebSocketPacket.h"
#include "sha1.h"
#include"../tcp/Buffer.h"
#include"base64.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
class WebSocketContext
{
public:
    enum class WebSocketState
    {
        kUnconnected,
        kHandshaked
    };
    WebSocketContext();
    ~WebSocketContext();
    void HandleHandshake(Buffer*buf,const std::string&server_key);
    void Reset();
    void ParseData(Buffer*buf,Buffer*output);
    void SetWebSocketHandshakeState();
    WebSocketState GetWebSocketState() const{return state_;}
    uint8_t GetReqOpcode() const{ return request_packet_.GetOpcode(); }

private:
    WebSocketState state_;
    WebSocketPacket request_packet_;
};
