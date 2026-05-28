#include "ipc/NamedPipeChannel.hpp"

#include "common/Errors.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace plazza::ipc {

namespace {

bool writeAllRaw(int fd, const std::uint8_t *data, std::size_t size, int &errnoOut) noexcept
{
    std::size_t written = 0;
    while (written < size) {
        const ssize_t n = ::write(fd, data + written, size - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            errnoOut = errno;
            return false;
        }
        if (n == 0) {
            errnoOut = 0;
            return false;
        }
        written += static_cast<std::size_t>(n);
    }
    return true;
}

enum class ReadOutcome { Ok, Eof, Error };

ReadOutcome readAllRaw(int fd, std::uint8_t *data, std::size_t size, int &errnoOut) noexcept
{
    std::size_t got = 0;
    while (got < size) {
        const ssize_t n = ::read(fd, data + got, size - got);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            errnoOut = errno;
            return ReadOutcome::Error;
        }
        if (n == 0) {
            errnoOut = 0;
            return got == 0 ? ReadOutcome::Eof : ReadOutcome::Error;
        }
        got += static_cast<std::size_t>(n);
    }
    return ReadOutcome::Ok;
}

void packLengthLE(std::uint32_t length, std::uint8_t out[4]) noexcept
{
    out[0] = static_cast<std::uint8_t>(length & 0xFFu);
    out[1] = static_cast<std::uint8_t>((length >> 8) & 0xFFu);
    out[2] = static_cast<std::uint8_t>((length >> 16) & 0xFFu);
    out[3] = static_cast<std::uint8_t>((length >> 24) & 0xFFu);
}

std::uint32_t unpackLengthLE(const std::uint8_t in[4]) noexcept
{
    return static_cast<std::uint32_t>(in[0])
        | (static_cast<std::uint32_t>(in[1]) << 8)
        | (static_cast<std::uint32_t>(in[2]) << 16)
        | (static_cast<std::uint32_t>(in[3]) << 24);
}

}

NamedPipeChannel::NamedPipeChannel() noexcept : _readFd(-1), _writeFd(-1) {}

NamedPipeChannel::NamedPipeChannel(int readFd, int writeFd) noexcept
    : _readFd(readFd), _writeFd(writeFd)
{}

NamedPipeChannel::~NamedPipeChannel()
{
    close();
}

NamedPipeChannel::NamedPipeChannel(NamedPipeChannel &&other) noexcept
    : _readFd(other._readFd), _writeFd(other._writeFd)
{
    other._readFd = -1;
    other._writeFd = -1;
}

NamedPipeChannel &NamedPipeChannel::operator=(NamedPipeChannel &&other) noexcept
{
    if (this != &other) {
        close();
        _readFd = other._readFd;
        _writeFd = other._writeFd;
        other._readFd = -1;
        other._writeFd = -1;
    }
    return *this;
}

void NamedPipeChannel::send(const std::vector<std::uint8_t> &payload)
{
    if (_writeFd < 0)
        throw IpcError("NamedPipeChannel::send: write side is closed", 0);
    if (payload.size() > std::numeric_limits<std::uint32_t>::max())
        throw IpcError("NamedPipeChannel::send: payload exceeds 4 GiB limit", 0);

    std::uint8_t header[4];
    packLengthLE(static_cast<std::uint32_t>(payload.size()), header);

    int errnoSnapshot = 0;
    if (!writeAllRaw(_writeFd, header, sizeof(header), errnoSnapshot))
        throw IpcError("NamedPipeChannel::send: failed to write length prefix: " +
            std::string(std::strerror(errnoSnapshot)), errnoSnapshot);
    if (!payload.empty() && !writeAllRaw(_writeFd, payload.data(), payload.size(), errnoSnapshot))
        throw IpcError("NamedPipeChannel::send: failed to write payload: " +
            std::string(std::strerror(errnoSnapshot)), errnoSnapshot);
}

std::optional<std::vector<std::uint8_t>> NamedPipeChannel::receive()
{
    if (_readFd < 0)
        throw IpcError("NamedPipeChannel::receive: read side is closed", 0);

    std::uint8_t header[4];
    int errnoSnapshot = 0;
    const auto headerOutcome = readAllRaw(_readFd, header, sizeof(header), errnoSnapshot);
    if (headerOutcome == ReadOutcome::Eof)
        return std::nullopt;
    if (headerOutcome == ReadOutcome::Error)
        throw IpcError("NamedPipeChannel::receive: short header: " +
            std::string(std::strerror(errnoSnapshot)), errnoSnapshot);

    const std::uint32_t length = unpackLengthLE(header);
    std::vector<std::uint8_t> payload(length);
    if (length > 0) {
        const auto bodyOutcome = readAllRaw(_readFd, payload.data(), length, errnoSnapshot);
        if (bodyOutcome != ReadOutcome::Ok)
            throw IpcError("NamedPipeChannel::receive: short payload (expected " +
                std::to_string(length) + " bytes): " +
                std::string(std::strerror(errnoSnapshot)), errnoSnapshot);
    }
    return payload;
}

void NamedPipeChannel::close()
{
    if (_readFd >= 0 && _readFd != _writeFd) {
        ::close(_readFd);
    }
    if (_writeFd >= 0) {
        ::close(_writeFd);
    }
    _readFd = -1;
    _writeFd = -1;
}

bool NamedPipeChannel::isOpen() const noexcept
{
    return _readFd >= 0 || _writeFd >= 0;
}

void NamedPipeChannel::createFifo(const std::string &path)
{
    if (::mkfifo(path.c_str(), 0600) == -1) {
        const int err = errno;
        if (err == EEXIST) {
            struct stat st{};
            if (::stat(path.c_str(), &st) == 0 && S_ISFIFO(st.st_mode))
                return;
        }
        throw IpcError("NamedPipeChannel::createFifo(" + path + "): " +
            std::string(std::strerror(err)), err);
    }
}

void NamedPipeChannel::unlinkFifo(const std::string &path) noexcept
{
    ::unlink(path.c_str());
}

int NamedPipeChannel::openFifoRead(const std::string &path)
{
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        const int err = errno;
        throw IpcError("NamedPipeChannel::openFifoRead(" + path + "): " +
            std::string(std::strerror(err)), err);
    }
    return fd;
}

int NamedPipeChannel::openFifoWrite(const std::string &path)
{
    const int fd = ::open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        const int err = errno;
        throw IpcError("NamedPipeChannel::openFifoWrite(" + path + "): " +
            std::string(std::strerror(err)), err);
    }
    return fd;
}

}
