#pragma once

#include "ipc/Messages.hpp"
#include "parser/Parser.hpp"
#include "reception/KitchenHandle.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

namespace plazza::reception {

class Dispatcher {
public:
    using SpawnCallback = std::function<KitchenHandle *()>;

    Dispatcher(std::vector<KitchenHandle *> handles, SpawnCallback spawnIfNeeded);

    Dispatcher(const Dispatcher &) = delete;
    Dispatcher &operator=(const Dispatcher &) = delete;

    void setHandles(std::vector<KitchenHandle *> handles);

    std::vector<std::uint64_t> dispatch(const parser::ParsedOrder &order);
    std::uint64_t dispatchOne(pizza::PizzaType type, pizza::PizzaSize size);

    [[nodiscard]] std::uint64_t nextOrderId() const noexcept { return _nextOrderId.load(); }

private:
    [[nodiscard]] KitchenHandle *pickLeastLoaded() const;
    [[nodiscard]] KitchenHandle *acquireTarget();

    std::vector<KitchenHandle *> _handles;
    SpawnCallback _spawnIfNeeded;
    std::atomic<std::uint64_t> _nextOrderId;
};

}
