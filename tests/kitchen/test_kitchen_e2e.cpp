#include <criterion/criterion.h>

#include "ipc/Messages.hpp"
#include "ipc/NamedPipeChannel.hpp"
#include "ipc/Serializer.hpp"
#include "kitchen/Kitchen.hpp"
#include "pizza/Types.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

using plazza::ipc::decode;
using plazza::ipc::Deserializer;
using plazza::ipc::encode;
using plazza::ipc::Message;
using plazza::ipc::MessageType;
using plazza::ipc::NamedPipeChannel;
using plazza::ipc::OrderMsg;
using plazza::ipc::PizzaDoneMsg;
using plazza::ipc::Serializer;
using plazza::ipc::ShutdownMsg;
using plazza::ipc::StatusReplyMsg;
using plazza::ipc::StatusRequestMsg;
using plazza::ipc::typeOf;
using plazza::kitchen::Kitchen;
using plazza::kitchen::KitchenConfig;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;
using namespace std::chrono_literals;

namespace {

struct PipePair {
    int parentRead = -1;
    int parentWrite = -1;
    int childRead = -1;
    int childWrite = -1;
};

PipePair makePipePair()
{
    int p2c[2];
    int c2p[2];
    cr_assert_eq(::pipe(p2c), 0, "pipe p2c failed: %s", std::strerror(errno));
    cr_assert_eq(::pipe(c2p), 0, "pipe c2p failed: %s", std::strerror(errno));
    PipePair pair;
    pair.parentWrite = p2c[1];
    pair.parentRead  = c2p[0];
    pair.childRead   = p2c[0];
    pair.childWrite  = c2p[1];
    return pair;
}

void sendMsg(NamedPipeChannel &channel, const Message &message)
{
    Serializer s;
    encode(s, message);
    channel.send(s.take());
}

std::optional<Message> recvMsgWithTimeout(NamedPipeChannel &channel, std::chrono::milliseconds timeout)
{
    const int fd = channel.readFd();
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval tv;
    tv.tv_sec  = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    const int sel = ::select(fd + 1, &set, nullptr, nullptr, &tv);
    if (sel <= 0)
        return std::nullopt;
    auto frame = channel.receive();
    if (!frame)
        return std::nullopt;
    Deserializer d(std::move(*frame));
    return decode(d);
}

int runChild(int childRead, int childWrite, KitchenConfig config)
{
    std::signal(SIGPIPE, SIG_IGN);
    NamedPipeChannel channel(childRead, childWrite);
    Kitchen kitchen(config);
    kitchen.run(channel);
    return 0;
}

pid_t spawnKitchen(const PipePair &pair, KitchenConfig config)
{
    const pid_t pid = ::fork();
    cr_assert_geq(pid, 0, "fork failed: %s", std::strerror(errno));
    if (pid == 0) {
        ::close(pair.parentRead);
        ::close(pair.parentWrite);
        std::_Exit(runChild(pair.childRead, pair.childWrite, config));
    }
    ::close(pair.childRead);
    ::close(pair.childWrite);
    return pid;
}

void reapChild(pid_t pid, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        int status = 0;
        const pid_t r = ::waitpid(pid, &status, WNOHANG);
        if (r == pid)
            return;
        if (std::chrono::steady_clock::now() >= deadline) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            cr_assert(false, "child did not exit before timeout");
            return;
        }
        std::this_thread::sleep_for(10ms);
    }
}

KitchenConfig fastConfig()
{
    KitchenConfig cfg;
    cfg.id              = 7;
    cfg.cookCount       = 2;
    cfg.timeMultiplier  = 0.01;
    cfg.restockPeriod   = 20ms;
    cfg.idleTimeout     = 5000ms;
    cfg.stockInitial    = 50;
    return cfg;
}

}

Test(kitchen_e2e, single_order_yields_pizza_done, .timeout = 10)
{
    auto pipes = makePipePair();
    const pid_t child = spawnKitchen(pipes, fastConfig());

    NamedPipeChannel channel(pipes.parentRead, pipes.parentWrite);
    sendMsg(channel, Message{OrderMsg{1001, PizzaType::Margarita, PizzaSize::M}});

    auto reply = recvMsgWithTimeout(channel, 5s);
    cr_assert(reply.has_value(), "expected a PizzaDone reply");
    cr_assert_eq(typeOf(*reply), MessageType::PizzaDone);
    const auto &done = std::get<PizzaDoneMsg>(*reply);
    cr_assert_eq(done.orderId, 1001u);
    cr_assert_eq(done.kitchenId, 7);

    sendMsg(channel, Message{ShutdownMsg{}});
    reapChild(child, 5s);
}

Test(kitchen_e2e, status_request_returns_reply, .timeout = 10)
{
    auto pipes = makePipePair();
    const pid_t child = spawnKitchen(pipes, fastConfig());

    NamedPipeChannel channel(pipes.parentRead, pipes.parentWrite);
    sendMsg(channel, Message{StatusRequestMsg{}});

    auto reply = recvMsgWithTimeout(channel, 5s);
    cr_assert(reply.has_value());
    cr_assert_eq(typeOf(*reply), MessageType::StatusReply);
    const auto &status = std::get<StatusReplyMsg>(*reply);
    cr_assert_eq(status.kitchenId, 7);
    cr_assert_eq(status.cooksTotal, 2u);

    sendMsg(channel, Message{ShutdownMsg{}});
    reapChild(child, 5s);
}

Test(kitchen_e2e, multiple_orders_all_complete, .timeout = 10)
{
    auto pipes = makePipePair();
    const pid_t child = spawnKitchen(pipes, fastConfig());

    NamedPipeChannel channel(pipes.parentRead, pipes.parentWrite);
    constexpr int total = 8;
    for (int i = 0; i < total; ++i)
        sendMsg(channel, Message{OrderMsg{static_cast<std::uint64_t>(2000 + i), PizzaType::Margarita, PizzaSize::S}});

    std::vector<std::uint64_t> seen;
    seen.reserve(total);
    for (int i = 0; i < total; ++i) {
        auto reply = recvMsgWithTimeout(channel, 5s);
        cr_assert(reply.has_value(), "missing pizza %d", i);
        cr_assert_eq(typeOf(*reply), MessageType::PizzaDone);
        seen.push_back(std::get<PizzaDoneMsg>(*reply).orderId);
    }
    cr_assert_eq(static_cast<int>(seen.size()), total);

    sendMsg(channel, Message{ShutdownMsg{}});
    reapChild(child, 5s);
}

Test(kitchen_e2e, shutdown_message_stops_the_kitchen, .timeout = 10)
{
    auto pipes = makePipePair();
    const pid_t child = spawnKitchen(pipes, fastConfig());

    NamedPipeChannel channel(pipes.parentRead, pipes.parentWrite);
    sendMsg(channel, Message{ShutdownMsg{}});

    reapChild(child, 5s);
}

Test(kitchen_e2e, idle_timeout_self_terminates_kitchen, .timeout = 10)
{
    auto pipes = makePipePair();
    KitchenConfig cfg = fastConfig();
    cfg.idleTimeout = 150ms;
    const pid_t child = spawnKitchen(pipes, cfg);

    NamedPipeChannel channel(pipes.parentRead, pipes.parentWrite);
    sendMsg(channel, Message{OrderMsg{77, PizzaType::Margarita, PizzaSize::S}});
    auto done = recvMsgWithTimeout(channel, 5s);
    cr_assert(done.has_value());

    reapChild(child, 5s);
}
