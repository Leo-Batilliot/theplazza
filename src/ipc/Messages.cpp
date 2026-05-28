#include "ipc/Messages.hpp"

#include "common/Errors.hpp"

#include <string>
#include <type_traits>

namespace plazza::ipc {

MessageType typeOf(const Message &msg) noexcept
{
    return std::visit([](auto &&value) noexcept -> MessageType {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, OrderMsg>)
            return MessageType::Order;
        else if constexpr (std::is_same_v<T, PizzaDoneMsg>)
            return MessageType::PizzaDone;
        else if constexpr (std::is_same_v<T, StatusRequestMsg>)
            return MessageType::StatusRequest;
        else if constexpr (std::is_same_v<T, StatusReplyMsg>)
            return MessageType::StatusReply;
        else
            return MessageType::Shutdown;
    }, msg);
}

void encode(Serializer &out, const OrderMsg &msg)
{
    out.writeUInt64(msg.orderId);
    out.writeEnum(msg.type);
    out.writeEnum(msg.size);
}

void encode(Serializer &out, const PizzaDoneMsg &msg)
{
    out.writeUInt64(msg.orderId);
    out.writeInt32(msg.kitchenId);
}

void encode(Serializer &, const StatusRequestMsg &) {}

void encode(Serializer &out, const StatusReplyMsg &msg)
{
    out.writeInt32(msg.kitchenId);
    out.writeUInt32(msg.cooksTotal);
    out.writeUInt32(msg.cooksBusy);
    out.writeUInt32(msg.queueSize);
    for (const auto value : msg.stock)
        out.writeUInt32(value);
}

void encode(Serializer &, const ShutdownMsg &) {}

void encode(Serializer &out, const Message &msg)
{
    out.writeEnum(typeOf(msg));
    std::visit([&out](auto &&value) { encode(out, value); }, msg);
}

Message decode(Deserializer &in)
{
    const auto tag = in.readEnum<MessageType>();
    switch (tag) {
        case MessageType::Order: {
            OrderMsg msg{};
            msg.orderId = in.readUInt64();
            msg.type    = in.readEnum<pizza::PizzaType>();
            msg.size    = in.readEnum<pizza::PizzaSize>();
            return msg;
        }
        case MessageType::PizzaDone: {
            PizzaDoneMsg msg{};
            msg.orderId   = in.readUInt64();
            msg.kitchenId = in.readInt32();
            return msg;
        }
        case MessageType::StatusRequest:
            return StatusRequestMsg{};
        case MessageType::StatusReply: {
            StatusReplyMsg msg{};
            msg.kitchenId  = in.readInt32();
            msg.cooksTotal = in.readUInt32();
            msg.cooksBusy  = in.readUInt32();
            msg.queueSize  = in.readUInt32();
            for (auto &slot : msg.stock)
                slot = in.readUInt32();
            return msg;
        }
        case MessageType::Shutdown:
            return ShutdownMsg{};
    }
    throw IpcError("Messages::decode: unknown message tag " +
        std::to_string(static_cast<std::uint32_t>(tag)), 0);
}

}
