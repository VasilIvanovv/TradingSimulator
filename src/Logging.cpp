#include "Logging.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

LogStream::LogStream(LogLevel level, const char *fileName, int lineNumber)
    : m_logLevel(level),
      m_file(std::filesystem::path(fileName).filename().string()),
      m_line(lineNumber) {}

LogStream::~LogStream() {
  auto now = std::chrono::system_clock::now();
  auto localTime = std::chrono::zoned_time{std::chrono::current_zone(), now};

  std::ostringstream header;
  // Use std::format-style specifiers directly in the stream!
  header << "[" << std::format("{:%Y-%m-%d %H:%M:%S}", localTime) << "] "
         << getPrefix() << "[" << m_file << ":" << m_line << "] ";

  std::ofstream logFile("log.txt", std::ios::app);
  if (logFile.is_open()) {
    logFile << header.str() << m_stream.str() << std::endl;
  } else {
    std::cerr << "Failed to open log file!\n";
    std::cerr << header.str() << m_stream.str() << std::endl;
  }
}

std::string LogStream::getPrefix() const {
  switch (m_logLevel) {
  case LogLevel::Info:
    return "[INFO]    ";
  case LogLevel::Warning:
    return "[WARNING] ";
  case LogLevel::Error:
    return "[ERROR]   ";
  case LogLevel::Debug:
    return "[DEBUG]   ";
  default:
    return "[UNKNOWN] ";
  }
}
