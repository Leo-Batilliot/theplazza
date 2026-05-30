#pragma once

#include "ipc/Serializer.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace plazza::ipc {

class IpcChannel {
public:
    IpcChannel() = default;
    virtual ~IpcChannel() = default;

    IpcChannel(const IpcChannel &) = delete;
    IpcChannel &operator=(const IpcChannel &) = delete;
    IpcChannel(IpcChannel &&) = delete;
    IpcChannel &operator=(IpcChannel &&) = delete;

    virtual void send(const std::vector<std::uint8_t> &payload) = 0;
    virtual std::optional<std::vector<std::uint8_t>> receive() = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool isOpen() const noexcept = 0;
    [[nodiscard]] virtual int readFd() const noexcept { return -1; }

    IpcChannel &operator<<(const Serializer &serializer);
    IpcChannel &operator>>(Deserializer &out);
};

}
