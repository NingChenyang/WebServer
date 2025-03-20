#pragma once
#include <string>
#include<iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
enum class Method
{
    kInvalid,
    kGet,
    kPost,
    kHead,
    kPut,
    kDelete
};

enum class HttpStatusCode
{
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
};
enum class Version
{
    kUnknown,
    kHttp10,
    kHttp11
};
enum class HttpRequestPaseState
{
    kExpectRequestLine, // 请求行
    kExpectHeaders,
    kExpectBody,
    kGotAll,
};
// 用constexpr定义常量字符串
constexpr const char *kCRLF = "\r\n";
const std::string VersionToString(Version version);

const inline std::string GetFileExtension(const std::string &filename)
{
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos)
    {
        return "";
    }
    return filename.substr(dot_pos + 1);
    }

    const inline std::string GetMimeType(const std::string &extension)
    {
        if (extension == "html" || extension == "htm")
            return "text/html";
        if (extension == "css")
            return "text/css";
        if (extension == "js")
            return "application/javascript";
        if (extension == "json")
            return "application/json";
        if (extension == "png")
            return "image/png";
        if (extension == "jpg" || extension == "jpeg")
            return "image/jpeg";
        if (extension == "gif")
            return "image/gif";
        if (extension == "svg")
            return "image/svg+xml";
        if (extension == "ico")
            return "image/x-icon";
        if (extension == "pdf")
            return "application/pdf";
        if (extension == "txt")
            return "text/plain";
        return "application/octet-stream";
    }
    const std::string GetFileType(const std::string &filename);
    inline std::string ReadFile(const std::string &filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate); // ate表示打开时定位到文件末尾
        if (!file.is_open())
        {
            std::cout << "无法打开文件" << filepath << std::endl;
            return ""; // 文件打开失败返回空字符串
        }

        // 获取文件大小
        auto size = file.tellg();
        if (size == -1)
        {
            return "";
        }

        // 分配足够的空间
        std::string content;
        content.resize(size);

        // 回到文件开头
        file.seekg(0);

        // 读取整个文件
        if (!file.read(content.data(), size))
        {
            return "";
        }

        return content;
    }
