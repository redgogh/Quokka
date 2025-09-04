#ifndef QK_FORMAT_H_
#define QK_FORMAT_H_

#include <string>

/**
 * std::string format
 */
inline static std::string qk_format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    std::string buffer(n + 1, '\0');
    va_start(args, fmt);
    vsnprintf(std::data(buffer), std::size(buffer), fmt, args);
    va_end(args);
    buffer.resize(n);

    return buffer;
}

#endif /* QK_FORMAT_H_ */