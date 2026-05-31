#include "logging/Logger.hpp"

#include "common/Errors.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <ostream>
#include <sstream>

namespace plazza::logging {

namespace {

std::string timestamp()
{
    using clock = std::chrono::system_clock;
    const auto now    = clock::now();
    const auto secs   = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - secs).count();
    const std::time_t tt = clock::to_time_t(now);
    std::tm tmBuf{};
    ::localtime_r(&tt, &tmBuf);

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%H:%M:%S") << '.'
        << std::setw(3) << std::setfill('0') << millis;
    return oss.str();
}

}

Logger::Logger(std::ostream &sink, bool includeTimestamp)
    : _sink(&sink), _includeTimestamp(includeTimestamp)
{
}

Logger::Logger(const std::string &filePath, bool includeTimestamp)
    : _sink(nullptr), _includeTimestamp(includeTimestamp)
{
    _ownedFile = std::make_unique<std::ofstream>(filePath, std::ios::app);
    if (!_ownedFile->is_open())
        throw Error("Logger: failed to open file '" + filePath + "'");
    _sink = _ownedFile.get();
}

Logger::~Logger()
{
    close();
}

const char *Logger::levelName(Level level) noexcept
{
    switch (level) {
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "?";
}

void Logger::log(const std::string &message)
{
    log(Level::Info, message);
}

void Logger::log(Level level, const std::string &message)
{
    const std::string line = formatLine(level, message);
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_sink == nullptr)
        return;
    (*_sink) << line << '\n';
    _sink->flush();
}

void Logger::flush()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_sink != nullptr)
        _sink->flush();
}

void Logger::close()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_ownedFile && _ownedFile->is_open())
        _ownedFile->close();
    _sink = nullptr;
}

std::string Logger::formatLine(Level level, const std::string &message) const
{
    std::ostringstream oss;
    if (_includeTimestamp)
        oss << "[" << timestamp() << "] ";
    oss << "[" << levelName(level) << "] " << message;
    return oss.str();
}

}
