#include "reception/Dispatcher.hpp"

#include "common/Errors.hpp"

#include <utility>

namespace plazza::reception {

Dispatcher::Dispatcher(std::vector<KitchenHandle *> handles, SpawnCallback spawnIfNeeded)
    : _handles(std::move(handles)), _spawnIfNeeded(std::move(spawnIfNeeded)), _nextOrderId(1)
{
}

void Dispatcher::setHandles(std::vector<KitchenHandle *> handles)
{
    _handles = std::move(handles);
}

KitchenHandle *Dispatcher::pickLeastLoaded() const
{
    KitchenHandle *best = nullptr;
    for (auto *h : _handles) {
        if (!h || !h->isOpen() || !h->hasCapacity())
            continue;
        if (best == nullptr || h->inFlight() < best->inFlight())
            best = h;
    }
    return best;
}

KitchenHandle *Dispatcher::acquireTarget()
{
    if (auto *h = pickLeastLoaded(); h != nullptr)
        return h;
    if (_spawnIfNeeded) {
        if (auto *spawned = _spawnIfNeeded(); spawned != nullptr) {
            _handles.push_back(spawned);
            return spawned;
        }
    }
    throw Error("Dispatcher: no kitchen available and spawn failed");
}

std::uint64_t Dispatcher::dispatchOne(pizza::PizzaType type, pizza::PizzaSize size)
{
    KitchenHandle *target = acquireTarget();
    const std::uint64_t orderId = _nextOrderId.fetch_add(1);
    target->sendOrder(ipc::OrderMsg{orderId, type, size});
    target->noteOrderAccepted();
    return orderId;
}

std::vector<std::uint64_t> Dispatcher::dispatch(const parser::ParsedOrder &order)
{
    std::vector<std::uint64_t> ids;
    if (order.count <= 0)
        return ids;
    ids.reserve(static_cast<std::size_t>(order.count));
    for (int i = 0; i < order.count; ++i)
        ids.push_back(dispatchOne(order.type, order.size));
    return ids;
}

}
