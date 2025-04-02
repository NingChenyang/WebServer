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

    // 确保至少有2字节可读
    if (frameBuf->ReadableBytes() < 2)
    {
        return;
    }

    fin_ = (data[pos] >> 7) & 0x01;
    rsv1_ = (data[pos] >> 6) & 0x01;
    rsv2_ = (data[pos] >> 5) & 0x01;
    rsv3_ = (data[pos] >> 4) & 0x01;
    opcode_ = (data[pos] & 0x0F);
    pos++;

    mask_ = (data[pos] >> 7) & 0x01;
    payload_length_ = (data[pos] & 0x7F);
    pos++;

    // 处理扩展长度
    uint64_t length = payload_length_;
    if (payload_length_ == 126)
    {
        if (frameBuf->ReadableBytes() < pos + 2)
        {
            return;
        }
        uint16_t len16;
        memcpy(&len16, data + pos, 2);
        length = ntohs(len16);
        pos += 2;
    }
    else if (payload_length_ == 127)
    {
        if (frameBuf->ReadableBytes() < pos + 8)
        {
            return;
        }
        uint64_t len64;
        memcpy(&len64, data + pos, 8);
        length = be64toh(len64); // 使用网络字节序转换
        pos += 8;
    }

    // 检查剩余数据是否足够
    if (frameBuf->ReadableBytes() < pos + (mask_ ? 4 : 0) + length)
    {
        return;
    }

    // 处理掩码
    if (mask_)
    {
        memcpy(masking_key_, data + pos, 4);
        pos += 4;

        // 解码数据
        std::vector<char> decoded(length);
        for (uint64_t i = 0; i < length; i++)
        {
            decoded[i] = data[pos + i] ^ masking_key_[i % 4];
        }

        output->Append(decoded.data(), length);
    }
    else
    {
        output->Append(data + pos, length);
    }

    // 更新缓冲区位置
    frameBuf->Retrieve(pos + length);
}

void WebSocketPacket::EncodeFrame(Buffer *output, Buffer *data) const
{
    uint8_t byte1 = 0 | (fin_ << 7) | (rsv1_ << 6) | (rsv2_ << 5) | (rsv3_ << 4) | (opcode_ & 0x0F);
    output->Append((char *)&byte1, 1);

    uint8_t byte2 = 0 | (mask_ << 7);
    uint64_t length = data->ReadableBytes();

    if (length < 126)
    {
        byte2 |= length;
        output->Append((char *)&byte2, 1);
    }
    else if (length <= 65535)
    {
        byte2 |= 126;
        output->Append((char *)&byte2, 1);
        uint16_t len16 = htons(length);
        output->Append((char *)&len16, 2);
    }
    else
    {
        byte2 |= 127;
        output->Append((char *)&byte2, 1);
        uint64_t len64 = htobe64(length); // 使用网络字节序
        output->Append((char *)&len64, 8);
    }

    // 添加数据部分
    if (mask_ == 1)
    {
        // 服务端一般不需要mask
    }
    else
    {
        output->Append(data->Peek(), data->ReadableBytes());
    }
}
