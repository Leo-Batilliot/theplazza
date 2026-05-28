#include "ipc/IpcChannel.hpp"

#include "common/Errors.hpp"

#include <utility>

namespace plazza::ipc {

IpcChannel &IpcChannel::operator<<(const Serializer &serializer)
{
    send(serializer.data());
    return *this;
}

IpcChannel &IpcChannel::operator>>(Deserializer &out)
{
    auto payload = receive();
    if (!payload.has_value())
        throw IpcError("IpcChannel::operator>>: peer closed before delivering a message", 0);
    out = Deserializer(std::move(*payload));
    return *this;
}

}
