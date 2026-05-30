#pragma once

#include "ipc/IpcChannel.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace plazza::ipc {

class NamedPipeChannel : public IpcChannel {
public:
    NamedPipeChannel() noexcept;
    NamedPipeChannel(int readFd, int writeFd) noexcept;
    ~NamedPipeChannel() override;

    NamedPipeChannel(const NamedPipeChannel &) = delete;
    NamedPipeChannel &operator=(const NamedPipeChannel &) = delete;
    NamedPipeChannel(NamedPipeChannel &&other) noexcept;
    NamedPipeChannel &operator=(NamedPipeChannel &&other) noexcept;

    void send(const std::vector<std::uint8_t> &payload) override;
    std::optional<std::vector<std::uint8_t>> receive() override;
    void close() override;
    [[nodiscard]] bool isOpen() const noexcept override;

    [[nodiscard]] int readFd() const noexcept override { return _readFd; }
    [[nodiscard]] int writeFd() const noexcept { return _writeFd; }

    static void createFifo(const std::string &path);
    static void unlinkFifo(const std::string &path) noexcept;
    static int openFifoRead(const std::string &path);
    static int openFifoWrite(const std::string &path);

private:
    int _readFd;
    int _writeFd;
};

}
