#include "reception/KitchenHandle.hpp"

#include "common/Errors.hpp"
#include "ipc/Serializer.hpp"

#include <sys/select.h>

#include <cerrno>
#include <mutex>
#include <utility>

namespace plazza::reception {

KitchenHandle::KitchenHandle(std::int32_t id, std::unique_ptr<ipc::IpcChannel> channel,
    std::size_t maxConcurrent)
    : _id(id), _channel(std::move(channel)), _maxConcurrent(maxConcurrent), _inFlight(0)
{
    if (!_channel)
        throw Error("KitchenHandle: channel must not be null");
    if (maxConcurrent == 0)
        throw Error("KitchenHandle: maxConcurrent must be > 0");
}

void KitchenHandle::sendOrder(const ipc::OrderMsg &order)
{
    sendLocked(ipc::Message{order});
}

void KitchenHandle::sendStatusRequest()
{
    sendLocked(ipc::Message{ipc::StatusRequestMsg{}});
}

void KitchenHandle::sendShutdown()
{
    sendLocked(ipc::Message{ipc::ShutdownMsg{}});
}

void KitchenHandle::close()
{
    std::lock_guard<concurrency::Mutex> lock(_sendMutex);
    if (_channel)
        _channel->close();
}

void KitchenHandle::noteOrderAccepted()
{
    _inFlight.fetch_add(1);
}

void KitchenHandle::noteOrderCompleted()
{
    std::size_t prev = _inFlight.load();
    while (prev > 0 && !_inFlight.compare_exchange_weak(prev, prev - 1)) {}
}

bool KitchenHandle::isOpen() const noexcept
{
    return _channel && _channel->isOpen();
}

std::optional<ipc::Message> KitchenHandle::tryReceive(std::chrono::milliseconds timeout)
{
    if (!_channel || !_channel->isOpen())
        return std::nullopt;
    const int fd = _channel->readFd();
    if (fd >= 0) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        struct timeval tv;
        tv.tv_sec  = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        const int sel = ::select(fd + 1, &set, nullptr, nullptr, &tv);
        if (sel <= 0)
            return std::nullopt;
    }
    auto frame = _channel->receive();
    if (!frame)
        return std::nullopt;
    ipc::Deserializer in(std::move(*frame));
    return ipc::decode(in);
}

void KitchenHandle::sendLocked(const ipc::Message &message)
{
    ipc::Serializer out;
    ipc::encode(out, message);
    std::lock_guard<concurrency::Mutex> lock(_sendMutex);
    if (!_channel || !_channel->isOpen())
        throw IpcError("KitchenHandle::send: channel not open", 0);
    _channel->send(out.take());
}

}
