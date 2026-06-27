#pragma once
#include <sstream>
#include <string>

enum class LogLevel {
    Info,
    Warning,
    Error,
    Debug
};

class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line);
    ~LogStream();

    template <typename T>
    LogStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }

private:
    std::ostringstream m_stream;
    LogLevel           m_logLevel;
    std::string        m_file;
    int                m_line;

    std::string getPrefix() const;
};

// Usage: LOG(Info), LOG(Warning), LOG(Error), LOG(Debug)
#define LOG(level) LogStream(LogLevel::level, __FILE__, __LINE__)
