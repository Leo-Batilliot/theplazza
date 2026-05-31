#pragma once

#include "concurrency/Mutex.hpp"

#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <string>

namespace plazza::logging {

enum class Level : std::uint8_t {
    Info,
    Warn,
    Error,
};

class Logger {
public:
    explicit Logger(std::ostream &sink, bool includeTimestamp = true);
    explicit Logger(const std::string &filePath, bool includeTimestamp = true);
    ~Logger();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    void log(const std::string &message);
    void log(Level level, const std::string &message);
    void flush();
    void close();

    [[nodiscard]] bool isOpen() const noexcept { return _sink != nullptr; }

    static const char *levelName(Level level) noexcept;

private:
    std::string formatLine(Level level, const std::string &message) const;

    mutable concurrency::Mutex _mutex;
    std::ostream *_sink;
    std::unique_ptr<std::ofstream> _ownedFile;
    bool _includeTimestamp;
};

}
