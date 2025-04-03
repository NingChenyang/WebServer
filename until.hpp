#ifndef UNTIL_HPP
#define UNTIL_HPP

#include <string>
#include <sstream>
#include <iomanip>
// ...existing code...

// 当汉字数据通过路径传递时可能出现编码异常
// 添加 encode 函数：对字符串进行 URL 编码
inline std::string encode(const std::string &data)
{
    std::ostringstream encoded;
    for (unsigned char c : data)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded << c;
        }
        else
        {
            encoded << '%' << std::uppercase << std::setw(2) << std::setfill('0') << int(c);
        }
    }
    return encoded.str();
}

// 添加 decode 函数：对 URL 编码后的字符串进行译码
inline std::string decode(const std::string &data)
{
    std::ostringstream decoded;
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (data[i] == '%' && i + 2 < data.size())
        {
            std::istringstream iss(data.substr(i + 1, 2));
            int hex;
            if (iss >> std::hex >> hex)
            {
                decoded << static_cast<char>(hex);
                i += 2;
            }
            else
            {
                decoded << data[i];
            }
        }
        else
        {
            decoded << data[i];
        }
    }
    return decoded.str();
}

// ...existing code...
#endif // UNTIL_HPP
