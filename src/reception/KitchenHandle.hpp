#pragma once

#include "concurrency/Mutex.hpp"
#include "ipc/IpcChannel.hpp"
#include "ipc/Messages.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace plazza::reception {

class KitchenHandle {
public:
    KitchenHandle(std::int32_t id, std::unique_ptr<ipc::IpcChannel> channel,
        std::size_t maxConcurrent);
    ~KitchenHandle() = default;

    KitchenHandle(const KitchenHandle &) = delete;
    KitchenHandle &operator=(const KitchenHandle &) = delete;
    KitchenHandle(KitchenHandle &&) = delete;
    KitchenHandle &operator=(KitchenHandle &&) = delete;

    void sendOrder(const ipc::OrderMsg &order);
    void sendStatusRequest();
    void sendShutdown();
    void close();

    [[nodiscard]] std::optional<ipc::Message> tryReceive(std::chrono::milliseconds timeout);

    void noteOrderAccepted();
    void noteOrderCompleted();

    [[nodiscard]] std::int32_t id() const noexcept { return _id; }
    [[nodiscard]] std::size_t inFlight() const noexcept { return _inFlight.load(); }
    [[nodiscard]] std::size_t maxConcurrent() const noexcept { return _maxConcurrent; }
    [[nodiscard]] bool hasCapacity() const noexcept { return inFlight() < _maxConcurrent; }
    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] ipc::IpcChannel &channel() noexcept { return *_channel; }

private:
    void sendLocked(const ipc::Message &message);

    std::int32_t _id;
    std::unique_ptr<ipc::IpcChannel> _channel;
    std::size_t _maxConcurrent;
    std::atomic<std::size_t> _inFlight;
    concurrency::Mutex _sendMutex;
};

}
