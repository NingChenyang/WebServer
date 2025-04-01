#include "WebSocketContext.h"

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
WebSocketContext::WebSocketContext()
: state_(WebSocketState::kUnconnected),
  request_packet_()
{

}

WebSocketContext::~WebSocketContext()
{

}

void WebSocketContext::HandleHandshake(Buffer *buf, const std::string &server_key)
{
    buf->Append("HTTP/1.1 101 Switching Protocols\r\n");
    buf->Append("Connection: Upgrade\r\n");
    buf->Append("Sec-WebSocket-Accept: ");
    std::string serverkey=server_key;
    serverkey+=MAGIC_KEY;
    SHA1 sha;
    unsigned int digest[5];
    sha.Reset();
    sha<<serverkey.c_str();
    sha.Result(digest);
    for (int i = 0; i < 5; i++)
    {
        digest[i] = ntohl(digest[i]);
    }
    serverkey=base64_encode(reinterpret_cast<const unsigned char*>(digest), 20);
    serverkey+="\r\n";
    buf->Append(serverkey);
    buf->Append("Upgrade: websocket\r\n\r\n");
    
}

void WebSocketContext::Reset()
{
    request_packet_.Reset();
}

void WebSocketContext::ParseData(Buffer *buf, Buffer *output)
{
    request_packet_.DecodeFrame(buf, output);
}

void WebSocketContext::SetWebSocketHandshakeState()
{
    state_ = WebSocketState::kHandshaked;
}

