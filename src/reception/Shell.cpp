#include "reception/Shell.hpp"

#include "common/Errors.hpp"

#include <algorithm>
#include <cctype>
#include <istream>
#include <mutex>
#include <string>
#include <utility>

namespace plazza::reception {

namespace {

std::string trim(const std::string &input)
{
    auto first = input.begin();
    auto last  = input.end();
    while (first != last && std::isspace(static_cast<unsigned char>(*first)))
        ++first;
    while (last != first && std::isspace(static_cast<unsigned char>(*(last - 1))))
        --last;
    return std::string(first, last);
}

std::string toLower(const std::string &input)
{
    std::string out(input.size(), '\0');
    std::transform(input.begin(), input.end(), out.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return out;
}

}

Shell::Shell(std::istream &input, ShellCallbacks callbacks)
    : _input(input), _callbacks(std::move(callbacks)),
      _stopRequested(false), _running(false), _finished(false)
{
    if (!_callbacks.onOrder)
        throw Error("Shell: onOrder callback is required");
}

Shell::~Shell()
{
    join();
}

void Shell::start()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (_running.load())
            throw Error("Shell::start: already running");
        _stopRequested.store(false);
        _finished.store(false);
        _running.store(true);
    }
    _thread = concurrency::Thread([this] { loop(); });
}

void Shell::runUntilEof()
{
    if (_running.exchange(true))
        throw Error("Shell::runUntilEof: already running");
    _stopRequested.store(false);
    _finished.store(false);
    loop();
}

void Shell::requestStop()
{
    _stopRequested.store(true);
}

void Shell::join()
{
    if (_thread.joinable())
        _thread.join();
}

bool Shell::running() const
{
    return _running.load() && !_finished.load();
}

void Shell::processLine(const std::string &line, const ShellCallbacks &callbacks)
{
    const std::string trimmed = trim(line);
    if (trimmed.empty())
        return;
    if (toLower(trimmed) == statusCommand) {
        if (callbacks.onStatus)
            callbacks.onStatus();
        return;
    }
    try {
        parser::Parser parser(trimmed);
        for (const auto &order : parser.parse())
            callbacks.onOrder(order);
    } catch (const ParseError &error) {
        if (callbacks.onError)
            callbacks.onError(error.what());
    }
}

void Shell::loop()
{
    std::string line;
    while (!_stopRequested.load() && std::getline(_input, line))
        processLine(line, _callbacks);
    _running.store(false);
    _finished.store(true);
}

}
