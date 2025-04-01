#include "WebSocketPacket.h"

WebSocketPacket::WebSocketPacket()
    : fin_(1), // 1,代表不分包
      rsv1_(0),
      rsv2_(0),
      rsv3_(0),
      opcode_(1), // 默认发送文本帧
      mask_(0),
      payload_length_(0)
{
    std::fill(std::begin(masking_key_), std::end(masking_key_), 0);
}

WebSocketPacket::~WebSocketPacket()
{
}

void WebSocketPacket::Reset()
{
    fin_ = 1;
    rsv1_ = 0;
    rsv2_ = 0;
    rsv3_ = 0;
    opcode_ = 1;
    mask_ = 0;
    payload_length_ = 0;
    std::fill(std::begin(masking_key_), std::end(masking_key_), 0);
}

void WebSocketPacket::DecodeFrame(Buffer *frameBuf, Buffer *output)
{
    const char *data = frameBuf->Peek();
    int pos = 0;
    fin_ = (data[pos] >> 7) & 0x01;
    rsv1_ = (data[pos] >> 6) & 0x01;
    rsv2_ = (data[pos] >> 5) & 0x01;
    rsv3_ = (data[pos] >> 4) & 0x01;
    opcode_ = (data[pos] & 0x0F);
    pos++;
    mask_ = (data[pos] >> 7) & 0x01;
    payload_length_ = (data[pos] & 0x7F);
    pos++;

    // 数据长度，如果使用额外的字节需要进行大端（网络字节序）转小端
    if (payload_length_ == 126)
    {
        uint16_t length = 0;
        memcpy(&length, data + pos, 2);
        pos += 2;
        payload_length_ = ntohs(length);
    }
    else if (payload_length_ == 127)
    {
        uint64_t length = 0;
        memcpy(&length, data + pos, 8);
        pos += 8;
        payload_length_ = ((uint64_t)data[0] << 56) |
                          ((uint64_t)data[1] << 48) |
                          ((uint64_t)data[2] << 40) |
                          ((uint64_t)data[3] << 32) |
                          ((uint64_t)data[4] << 24) |
                          ((uint64_t)data[5] << 16) |
                          ((uint64_t)data[6] << 8) |
                          ((uint64_t)data[7]);
    }

    // 获取mask
    if (mask_ == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            masking_key_[i] = data[pos + i];
        }
        pos += 4;
    }
    if (mask_ == 1)
    {
        for (uint64_t i = 0; i < payload_length_; i++)
        {
            output->Append(data[pos + i] ^ masking_key_[i % 4], payload_length_);
        }
    }
    else
    {
        output->Append(data + pos, payload_length_);
    }
}

void WebSocketPacket::EncodeFrame(Buffer *output, Buffer *data) const
{
    uint8_t byte1 = 0 | (fin_ << 7) | (rsv1_ << 6) | (rsv2_ << 5) | (rsv3_ << 4) | (opcode_ & 0x0F);
    output->Append((char *)&byte1, 1);
    uint8_t byte2 = 0 | (mask_ << 7);
    int length = data->ReadableBytes();
    //处理长度
        byte2 |= length;
        output->Append((char *)&byte2, 1);
     if (length == 126)
    {
        
        uint16_t len16 = htons(length);
        output->Append((char *)&len16, 2);
    }
    else if(length ==127)
    {
        
        

        //进行64位小端转大端
        byte2 = (payload_length_ >> 56) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 48) & 0xFF;
        output->Append((char *)&byte2, 1);  
        byte2 = (payload_length_ >> 40) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 32) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 24) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 16) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 8) & 0xFF;
        output->Append((char *)&byte2, 1);
        byte2 = (payload_length_ >> 0) & 0xFF;
        output->Append((char *)&byte2, 1);
    }

    //服务端不会发送带有mask_key的帧
    if (mask_ == 1)
    {
        
    }
    else
    {
        output->Append(data->Peek(), data->ReadableBytes());
    }
}

void WebSocketPacket::AddFrameHeader(Buffer *output)
{
    payload_length_ = output->ReadableBytes() - 14;

    uint8_t onebyte = 0;
    onebyte |= (fin_ << 7);
    onebyte |= (rsv1_ << 6);
    onebyte |= (rsv2_ << 5);
    onebyte |= (rsv3_ << 4);
    onebyte |= (opcode_ & 0x0F);

    output->Append((char *)&onebyte, 1);

    onebyte = 0;
    // set mask flag
    onebyte = onebyte | (mask_ << 7);

    int length = payload_length_;

    if (length < 126)
    {
        onebyte |= length;
        output->Append((char *)&onebyte, 1);
    }
    else if (length == 126)
    {
        onebyte |= length;
        output->Append((char *)&onebyte, 1);

        auto len = htons(length);
        output->Append((char *)&len, 2);
    }
    else if (length == 127)
    {
        onebyte |= length;
        output->Append((char *)&onebyte, 1);

        // also can use htonll if you have it
        onebyte = (payload_length_ >> 56) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 48) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 40) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 32) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 24) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 16) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = (payload_length_ >> 8) & 0xFF;
        output->Append((char *)&onebyte, 1);
        onebyte = payload_length_ & 0XFF;
        output->Append((char *)&onebyte, 1);
    }
}
