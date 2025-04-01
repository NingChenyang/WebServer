#include <string>
#include <iostream>
#include <cstring>     
#include <algorithm>   
#include <arpa/inet.h> 
#include <cstdint>
#include"../tcp/Buffer.h"
#pragma once
class Buffer;
enum  WebSocketOpcode :uint8_t
{
    kContinuationFrame = 0x0,
    kTextFrame = 0x1,
    kBinaryFrame = 0x2,
    kCloseFrame = 0x8,
    kPingFrame = 0x9,
    kPongFrame = 0xA
};
class WebSocketPacket
{
public:
    WebSocketPacket();
    ~WebSocketPacket();

    //重置数据包
    void Reset();
    //解析数据包
    void DecodeFrame(Buffer *frameBuf, Buffer *output);
    //编码数据包
    void EncodeFrame(Buffer *output, Buffer *data) const;

    void AddFrameHeader(Buffer *output);

    uint8_t GetFin() const { return fin_; }
    uint8_t GetRsv1() const { return rsv1_; }
    uint8_t GetRsv2() const { return rsv2_; }
    uint8_t GetRsv3() const { return rsv3_; }
    uint8_t GetOpcode() const { return opcode_; }
    uint8_t GetMask() const { return mask_; }
    uint64_t GetPayloadLength() const { return payload_length_; }

    void SetFin(uint8_t fin) { fin_ = fin; }
    void SetRsv1(uint8_t rsv1) { rsv1_ = rsv1; }
    void SetRsv2(uint8_t rsv2) { rsv2_ = rsv2; }
    void SetRsv3(uint8_t rsv3) { rsv3_ = rsv3; }
    void SetOpcode(uint8_t opcode) { opcode_ = opcode; }
    void SetMask(uint8_t mask) { mask_ = mask; }
    void SetPayloadLength(uint64_t length) { payload_length_ = length; }

private:
    uint8_t fin_ : 1; // FIN位
    uint8_t rsv1_ : 1; // RSV1位
    uint8_t rsv2_ : 1; // RSV2位
    uint8_t rsv3_ : 1; // RSV3位
    uint8_t opcode_ : 4; // 操作码
    uint8_t mask_ : 1; // 掩码位
    uint8_t masking_key_[4]; // 掩码密钥
    uint8_t payload_length_; // 有效载荷长度

};
