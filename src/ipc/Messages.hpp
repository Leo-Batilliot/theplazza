#pragma once

#include "ipc/Serializer.hpp"
#include "pizza/Types.hpp"

#include <array>
#include <cstdint>
#include <variant>

namespace plazza::ipc {

enum class MessageType : std::uint32_t {
    Order         = 1,
    PizzaDone     = 2,
    StatusRequest = 3,
    StatusReply   = 4,
    Shutdown      = 5,
};

struct OrderMsg {
    std::uint64_t orderId;
    pizza::PizzaType type;
    pizza::PizzaSize size;
};

struct PizzaDoneMsg {
    std::uint64_t orderId;
    std::int32_t kitchenId;
};

struct StatusRequestMsg {};

struct StatusReplyMsg {
    std::int32_t kitchenId;
    std::uint32_t cooksTotal;
    std::uint32_t cooksBusy;
    std::uint32_t queueSize;
    std::array<std::uint32_t, pizza::ingredientCount> stock;
};

struct ShutdownMsg {};

using Message = std::variant<OrderMsg, PizzaDoneMsg, StatusRequestMsg, StatusReplyMsg, ShutdownMsg>;

[[nodiscard]] MessageType typeOf(const Message &msg) noexcept;

void encode(Serializer &out, const OrderMsg &msg);
void encode(Serializer &out, const PizzaDoneMsg &msg);
void encode(Serializer &out, const StatusRequestMsg &msg);
void encode(Serializer &out, const StatusReplyMsg &msg);
void encode(Serializer &out, const ShutdownMsg &msg);
void encode(Serializer &out, const Message &msg);

[[nodiscard]] Message decode(Deserializer &in);

}
