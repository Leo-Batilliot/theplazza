#pragma once

#include "concurrency/Mutex.hpp"
#include "concurrency/Thread.hpp"
#include "parser/Parser.hpp"

#include <atomic>
#include <functional>
#include <iosfwd>
#include <string>

namespace plazza::reception {

struct ShellCallbacks {
    std::function<void(const parser::ParsedOrder &)> onOrder;
    std::function<void()> onStatus;
    std::function<void(const std::string &)> onError;
};

class Shell {
public:
    static constexpr const char *statusCommand = "status";

    Shell(std::istream &input, ShellCallbacks callbacks);
    ~Shell();

    Shell(const Shell &) = delete;
    Shell &operator=(const Shell &) = delete;
    Shell(Shell &&) = delete;
    Shell &operator=(Shell &&) = delete;

    void start();
    void runUntilEof();
    void requestStop();
    void join();

    [[nodiscard]] bool running() const;
    [[nodiscard]] bool finished() const noexcept { return _finished.load(); }

    static void processLine(const std::string &line, const ShellCallbacks &callbacks);

private:
    void loop();

    std::istream &_input;
    ShellCallbacks _callbacks;
    concurrency::Thread _thread;
    mutable concurrency::Mutex _mutex;
    std::atomic<bool> _stopRequested;
    std::atomic<bool> _running;
    std::atomic<bool> _finished;
};

}
