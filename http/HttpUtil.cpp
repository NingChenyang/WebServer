#include "HttpUtil.h"

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

const std::string GetFileType(const std::string &filename)
{
    return GetMimeType(GetFileExtension(filename));
}
