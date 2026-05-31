#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "ipc/Messages.hpp"
#include "ipc/NamedPipeChannel.hpp"
#include "ipc/Serializer.hpp"
#include "parser/Parser.hpp"
#include "pizza/Types.hpp"
#include "reception/Dispatcher.hpp"
#include "reception/KitchenHandle.hpp"

#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

using plazza::Error;
using plazza::ipc::decode;
using plazza::ipc::Deserializer;
using plazza::ipc::Message;
using plazza::ipc::MessageType;
using plazza::ipc::NamedPipeChannel;
using plazza::ipc::OrderMsg;
using plazza::ipc::ShutdownMsg;
using plazza::ipc::StatusRequestMsg;
using plazza::ipc::typeOf;
using plazza::parser::ParsedOrder;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;
using plazza::reception::Dispatcher;
using plazza::reception::KitchenHandle;
using namespace std::chrono_literals;

namespace {

struct Endpoints {
    std::unique_ptr<NamedPipeChannel> reception;
    std::unique_ptr<NamedPipeChannel> kitchen;
};

Endpoints makeEndpoints()
{
    int r2k[2];
    int k2r[2];
    cr_assert_eq(::pipe(r2k), 0);
    cr_assert_eq(::pipe(k2r), 0);
    Endpoints e;
    e.reception = std::make_unique<NamedPipeChannel>(k2r[0], r2k[1]);
    e.kitchen   = std::make_unique<NamedPipeChannel>(r2k[0], k2r[1]);
    return e;
}

Message readOne(NamedPipeChannel &ch)
{
    auto frame = ch.receive();
    cr_assert(frame.has_value());
    Deserializer d(std::move(*frame));
    return decode(d);
}

}

Test(kitchen_handle_basic, sending_order_writes_to_channel, .timeout = 5)
{
    auto ep = makeEndpoints();
    auto handle = std::make_unique<KitchenHandle>(1, std::move(ep.reception), 4);

    handle->sendOrder(OrderMsg{99, PizzaType::Margarita, PizzaSize::M});

    auto msg = readOne(*ep.kitchen);
    cr_assert_eq(typeOf(msg), MessageType::Order);
}

Test(kitchen_handle_basic, in_flight_tracking)
{
    auto ep = makeEndpoints();
    KitchenHandle handle(1, std::move(ep.reception), 3);

    cr_assert_eq(handle.inFlight(), 0u);
    cr_assert(handle.hasCapacity());

    handle.noteOrderAccepted();
    handle.noteOrderAccepted();
    cr_assert_eq(handle.inFlight(), 2u);
    cr_assert(handle.hasCapacity());

    handle.noteOrderAccepted();
    cr_assert_not(handle.hasCapacity());

    handle.noteOrderCompleted();
    cr_assert(handle.hasCapacity());
    handle.noteOrderCompleted();
    handle.noteOrderCompleted();
    handle.noteOrderCompleted();
    cr_assert_eq(handle.inFlight(), 0u);
}

Test(kitchen_handle_basic, sending_after_close_throws)
{
    auto ep = makeEndpoints();
    KitchenHandle handle(1, std::move(ep.reception), 4);

    handle.close();
    cr_assert_throw(handle.sendOrder(OrderMsg{1, PizzaType::Margarita, PizzaSize::M}),
        plazza::IpcError);
}

Test(kitchen_handle_basic, try_receive_returns_message, .timeout = 5)
{
    auto ep = makeEndpoints();
    KitchenHandle handle(1, std::move(ep.reception), 4);

    plazza::ipc::Serializer s;
    plazza::ipc::encode(s, Message{ShutdownMsg{}});
    ep.kitchen->send(s.take());

    auto msg = handle.tryReceive(1000ms);
    cr_assert(msg.has_value());
    cr_assert_eq(typeOf(*msg), MessageType::Shutdown);
}

Test(kitchen_handle_basic, try_receive_times_out)
{
    auto ep = makeEndpoints();
    KitchenHandle handle(1, std::move(ep.reception), 4);

    auto msg = handle.tryReceive(50ms);
    cr_assert_not(msg.has_value());
}

Test(dispatcher_basic, single_handle_receives_orders, .timeout = 5)
{
    auto ep = makeEndpoints();
    auto h = std::make_unique<KitchenHandle>(1, std::move(ep.reception), 4);
    Dispatcher dispatcher({h.get()}, nullptr);

    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Regina, PizzaSize::L, 3});
    cr_assert_eq(ids.size(), 3u);
    cr_assert_eq(h->inFlight(), 3u);

    for (int i = 0; i < 3; ++i) {
        auto msg = readOne(*ep.kitchen);
        cr_assert_eq(typeOf(msg), MessageType::Order);
        const auto &order = std::get<OrderMsg>(msg);
        cr_assert_eq(order.type, PizzaType::Regina);
    }
}

Test(dispatcher_lb, balances_load_across_two_kitchens, .timeout = 5)
{
    auto epA = makeEndpoints();
    auto epB = makeEndpoints();
    auto a = std::make_unique<KitchenHandle>(1, std::move(epA.reception), 4);
    auto b = std::make_unique<KitchenHandle>(2, std::move(epB.reception), 4);
    Dispatcher dispatcher({a.get(), b.get()}, nullptr);

    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 4});
    cr_assert_eq(ids.size(), 4u);
    cr_assert_eq(a->inFlight(), 2u);
    cr_assert_eq(b->inFlight(), 2u);
}

Test(dispatcher_lb, full_kitchen_is_skipped, .timeout = 5)
{
    auto epA = makeEndpoints();
    auto epB = makeEndpoints();
    auto a = std::make_unique<KitchenHandle>(1, std::move(epA.reception), 1);
    auto b = std::make_unique<KitchenHandle>(2, std::move(epB.reception), 4);
    a->noteOrderAccepted();
    Dispatcher dispatcher({a.get(), b.get()}, nullptr);

    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 3});
    cr_assert_eq(b->inFlight(), 3u);
    cr_assert_eq(a->inFlight(), 1u);
}

Test(dispatcher_spawn, calls_spawn_when_no_capacity, .timeout = 5)
{
    auto epA = makeEndpoints();
    auto epB = makeEndpoints();
    auto a = std::make_unique<KitchenHandle>(1, std::move(epA.reception), 1);
    a->noteOrderAccepted();
    auto b = std::make_unique<KitchenHandle>(2, std::move(epB.reception), 4);

    int spawnCalls = 0;
    Dispatcher dispatcher({a.get()}, [&]() -> KitchenHandle * {
        ++spawnCalls;
        return b.get();
    });

    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 2});
    cr_assert_eq(spawnCalls, 1);
    cr_assert_eq(b->inFlight(), 2u);
}

Test(dispatcher_error, no_handles_and_no_spawn_throws)
{
    Dispatcher dispatcher({}, nullptr);
    cr_assert_throw(dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 1}),
        Error);
}

Test(dispatcher_error, spawn_returning_null_throws)
{
    Dispatcher dispatcher({}, []() -> KitchenHandle * { return nullptr; });
    cr_assert_throw(dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 1}),
        Error);
}

Test(dispatcher_basic, zero_count_dispatches_nothing)
{
    Dispatcher dispatcher({}, nullptr);
    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 0});
    cr_assert_eq(ids.size(), 0u);
}

Test(dispatcher_basic, order_ids_are_monotonic_and_unique)
{
    auto ep = makeEndpoints();
    auto h = std::make_unique<KitchenHandle>(1, std::move(ep.reception), 100);
    Dispatcher dispatcher({h.get()}, nullptr);

    auto ids = dispatcher.dispatch(ParsedOrder{PizzaType::Margarita, PizzaSize::M, 5});
    for (std::size_t i = 1; i < ids.size(); ++i)
        cr_assert(ids[i] > ids[i - 1]);
}
