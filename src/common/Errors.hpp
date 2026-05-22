#pragma once

#include <exception>
#include <string>
#include <utility>

namespace plazza {

class Error : public std::exception {
public:
    explicit Error(std::string msg) : _msg(std::move(msg)) {}

    const char *what() const noexcept override { return _msg.c_str(); }

private:
    std::string _msg;
};

class CliError : public Error {
public:
    using Error::Error;
};

class ParseError : public Error {
public:
    using Error::Error;
};

class KitchenError : public Error {
public:
    using Error::Error;
};

class StockError : public Error {
public:
    using Error::Error;
};

class ProcessError : public Error {
public:
    using Error::Error;
};

class IpcError : public Error {
public:
    IpcError(std::string msg, const int errnoSnapshot) : Error(std::move(msg)), _errno(errnoSnapshot) {}

    [[nodiscard]] int errnoSnapshot() const noexcept { return _errno; }

private:
    int _errno;
};

}
