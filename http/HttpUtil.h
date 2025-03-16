#pragma once
#include <string>

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
    const std::string VersionToString(Version version)
    {
        switch (version)
        {
        case Version::kHttp10:
            return "HTTP/1.0";
        case Version::kHttp11:
            return "HTTP/1.1";
        default:
            return "Unknown";
        }
    }