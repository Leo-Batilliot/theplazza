#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "ipc/Messages.hpp"
#include "ipc/NamedPipeChannel.hpp"
#include "ipc/Serializer.hpp"
#include "pizza/Types.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <variant>
#include <vector>

using plazza::IpcError;
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
using plazza::pizza::ingredientCount;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;

namespace {

void ignoreSigpipe()
{
    std::signal(SIGPIPE, SIG_IGN);
}

NamedPipeChannel makeLoopback()
{
    int fds[2];
    cr_assert_eq(::pipe(fds), 0, "pipe() failed: %s", std::strerror(errno));
    return NamedPipeChannel(fds[0], fds[1]);
}

std::string uniqueFifoPath(const char *tag)
{
    static int counter = 0;
    return std::string("/tmp/plazza-test-") + tag + "-" +
        std::to_string(::getpid()) + "-" + std::to_string(++counter);
}

}

Test(messages_wire, order_round_trip)
{
    Serializer s;
    encode(s, Message{OrderMsg{42, PizzaType::Regina, PizzaSize::XXL}});

    Deserializer d(s.take());
    const Message decoded = decode(d);
    cr_assert_eq(typeOf(decoded), MessageType::Order);
    const auto &msg = std::get<OrderMsg>(decoded);
    cr_assert_eq(msg.orderId, 42u);
    cr_assert_eq(msg.type, PizzaType::Regina);
    cr_assert_eq(msg.size, PizzaSize::XXL);
    cr_assert(d.eof());
}

Test(messages_wire, pizza_done_round_trip)
{
    Serializer s;
    encode(s, Message{PizzaDoneMsg{0x0123456789ABCDEFull, 4321}});

    Deserializer d(s.take());
    const Message decoded = decode(d);
    const auto &msg = std::get<PizzaDoneMsg>(decoded);
    cr_assert_eq(msg.orderId, 0x0123456789ABCDEFull);
    cr_assert_eq(msg.kitchenId, 4321);
}

Test(messages_wire, status_request_round_trip)
{
    Serializer s;
    encode(s, Message{StatusRequestMsg{}});

    Deserializer d(s.take());
    const Message decoded = decode(d);
    cr_assert_eq(typeOf(decoded), MessageType::StatusRequest);
}

Test(messages_wire, status_reply_round_trip)
{
    StatusReplyMsg original{};
    original.kitchenId = -7;
    original.cooksTotal = 5;
    original.cooksBusy = 3;
    original.queueSize = 12;
    for (std::uint32_t i = 0; i < ingredientCount; ++i)
        original.stock[i] = i * 10;

    Serializer s;
    encode(s, Message{original});

    Deserializer d(s.take());
    const Message msg = decode(d);
    const auto &decoded = std::get<StatusReplyMsg>(msg);
    cr_assert_eq(decoded.kitchenId, -7);
    cr_assert_eq(decoded.cooksTotal, 5u);
    cr_assert_eq(decoded.cooksBusy, 3u);
    cr_assert_eq(decoded.queueSize, 12u);
    for (std::uint32_t i = 0; i < ingredientCount; ++i)
        cr_assert_eq(decoded.stock[i], i * 10);
    cr_assert(d.eof());
}

Test(messages_wire, shutdown_round_trip)
{
    Serializer s;
    encode(s, Message{ShutdownMsg{}});

    Deserializer d(s.take());
    const Message decoded = decode(d);
    cr_assert_eq(typeOf(decoded), MessageType::Shutdown);
}

Test(messages_wire, unknown_tag_throws)
{
    Serializer s;
    s.writeUInt32(99);

    Deserializer d(s.take());
    cr_assert_throw((void) decode(d), IpcError);
}

Test(messages_wire, type_tag_is_first_field_on_wire)
{
    Serializer s;
    encode(s, Message{OrderMsg{1, PizzaType::Margarita, PizzaSize::M}});

    const auto &bytes = s.data();
    cr_assert_geq(bytes.size(), 4u);
    cr_assert_eq(bytes[0], static_cast<std::uint8_t>(MessageType::Order));
    cr_assert_eq(bytes[1], 0u);
    cr_assert_eq(bytes[2], 0u);
    cr_assert_eq(bytes[3], 0u);
}

Test(named_pipe_loopback, single_message_round_trip)
{
    auto channel = makeLoopback();

    Serializer s;
    encode(s, Message{OrderMsg{1, PizzaType::Fantasia, PizzaSize::L}});
    channel << s;

    Deserializer d{std::vector<std::uint8_t>{}};
    channel >> d;
    const Message decoded = decode(d);
    const auto &msg = std::get<OrderMsg>(decoded);
    cr_assert_eq(msg.orderId, 1u);
    cr_assert_eq(msg.type, PizzaType::Fantasia);
    cr_assert_eq(msg.size, PizzaSize::L);
}

Test(named_pipe_loopback, three_messages_preserve_order)
{
    auto channel = makeLoopback();

    Serializer a, b, c;
    encode(a, Message{OrderMsg{1, PizzaType::Regina, PizzaSize::S}});
    encode(b, Message{OrderMsg{2, PizzaType::Margarita, PizzaSize::M}});
    encode(c, Message{OrderMsg{3, PizzaType::Americana, PizzaSize::XL}});
    channel << a;
    channel << b;
    channel << c;

    Deserializer d1{std::vector<std::uint8_t>{}};
    Deserializer d2{std::vector<std::uint8_t>{}};
    Deserializer d3{std::vector<std::uint8_t>{}};
    channel >> d1;
    channel >> d2;
    channel >> d3;

    const Message m1 = decode(d1);
    const Message m2 = decode(d2);
    const Message m3 = decode(d3);
    cr_assert_eq(std::get<OrderMsg>(m1).orderId, 1u);
    cr_assert_eq(std::get<OrderMsg>(m2).orderId, 2u);
    cr_assert_eq(std::get<OrderMsg>(m3).orderId, 3u);
}

Test(named_pipe_loopback, empty_payload_round_trip)
{
    auto channel = makeLoopback();

    channel.send({});
    const auto received = channel.receive();
    cr_assert(received.has_value());
    cr_assert_eq(received->size(), 0u);
}

Test(named_pipe_loopback, multi_kb_payload_round_trip)
{
    auto channel = makeLoopback();

    std::vector<std::uint8_t> payload(4096);
    for (std::size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<std::uint8_t>(i & 0xFFu);

    channel.send(payload);
    const auto received = channel.receive();
    cr_assert(received.has_value());
    cr_assert_eq(received->size(), payload.size());
    cr_assert_eq(std::memcmp(received->data(), payload.data(), payload.size()), 0);
}

Test(named_pipe_loopback, framing_uses_length_prefix)
{
    auto channel = makeLoopback();

    channel.send({0xDE, 0xAD});

    std::uint8_t header[4];
    cr_assert_eq(::read(channel.readFd(), header, 4), 4);
    cr_assert_eq(header[0], 2u);
    cr_assert_eq(header[1], 0u);
    cr_assert_eq(header[2], 0u);
    cr_assert_eq(header[3], 0u);

    std::uint8_t body[2];
    cr_assert_eq(::read(channel.readFd(), body, 2), 2);
    cr_assert_eq(body[0], 0xDEu);
    cr_assert_eq(body[1], 0xADu);
}

Test(named_pipe_loopback, receive_returns_nullopt_after_writer_closes)
{
    int fds[2];
    cr_assert_eq(::pipe(fds), 0);

    NamedPipeChannel reader(fds[0], -1);
    ::close(fds[1]);

    const auto result = reader.receive();
    cr_assert(!result.has_value());
}

Test(named_pipe_loopback, send_after_close_throws, .init = ignoreSigpipe)
{
    auto channel = makeLoopback();
    channel.close();

    cr_assert_not(channel.isOpen());
    cr_assert_throw(channel.send({1, 2, 3}), IpcError);
}

Test(named_pipe_loopback, receive_after_close_throws)
{
    auto channel = makeLoopback();
    channel.close();

    cr_assert_throw((void) channel.receive(), IpcError);
}

Test(named_pipe_loopback, short_payload_after_header_throws, .init = ignoreSigpipe)
{
    int fds[2];
    cr_assert_eq(::pipe(fds), 0);

    NamedPipeChannel reader(fds[0], -1);

    const std::uint8_t header[4] = {0x10, 0x00, 0x00, 0x00};
    const std::uint8_t partial[2] = {0xAA, 0xBB};
    cr_assert_eq(::write(fds[1], header, 4), 4);
    cr_assert_eq(::write(fds[1], partial, 2), 2);
    ::close(fds[1]);

    cr_assert_throw((void) reader.receive(), IpcError);
}

Test(named_pipe_fork, parent_sends_order_child_replies_pizza_done, .init = ignoreSigpipe)
{
    int p2c[2];
    int c2p[2];
    cr_assert_eq(::pipe(p2c), 0);
    cr_assert_eq(::pipe(c2p), 0);

    const pid_t pid = ::fork();
    cr_assert_geq(pid, 0);

    if (pid == 0) {
        ::close(p2c[1]);
        ::close(c2p[0]);
        NamedPipeChannel child(p2c[0], c2p[1]);

        Deserializer in{std::vector<std::uint8_t>{}};
        try {
            child >> in;
        } catch (...) {
            std::_Exit(2);
        }
        Message msg;
        try {
            msg = decode(in);
        } catch (...) {
            std::_Exit(3);
        }
        if (typeOf(msg) != MessageType::Order)
            std::_Exit(4);
        const auto &order = std::get<OrderMsg>(msg);
        if (order.orderId != 7 || order.type != PizzaType::Regina || order.size != PizzaSize::XXL)
            std::_Exit(5);

        Serializer out;
        encode(out, Message{PizzaDoneMsg{order.orderId, static_cast<std::int32_t>(::getpid())}});
        try {
            child << out;
        } catch (...) {
            std::_Exit(6);
        }
        std::_Exit(0);
    }

    ::close(p2c[0]);
    ::close(c2p[1]);
    NamedPipeChannel parent(c2p[0], p2c[1]);

    Serializer out;
    encode(out, Message{OrderMsg{7, PizzaType::Regina, PizzaSize::XXL}});
    parent << out;

    Deserializer in{std::vector<std::uint8_t>{}};
    parent >> in;
    const Message decoded = decode(in);
    const auto &done = std::get<PizzaDoneMsg>(decoded);
    cr_assert_eq(done.orderId, 7u);
    cr_assert_eq(done.kitchenId, pid);

    int status = 0;
    cr_assert_eq(::waitpid(pid, &status, 0), pid);
    cr_assert(WIFEXITED(status));
    cr_assert_eq(WEXITSTATUS(status), 0);
}

Test(named_pipe_fork, eof_propagates_after_child_exits, .init = ignoreSigpipe)
{
    int p2c[2];
    int c2p[2];
    cr_assert_eq(::pipe(p2c), 0);
    cr_assert_eq(::pipe(c2p), 0);

    const pid_t pid = ::fork();
    cr_assert_geq(pid, 0);

    if (pid == 0) {
        ::close(p2c[1]);
        ::close(c2p[0]);
        ::close(p2c[0]);
        ::close(c2p[1]);
        std::_Exit(0);
    }

    ::close(p2c[0]);
    ::close(c2p[1]);
    NamedPipeChannel parent(c2p[0], p2c[1]);

    int status = 0;
    ::waitpid(pid, &status, 0);

    const auto received = parent.receive();
    cr_assert(!received.has_value());
}

Test(named_pipe_fifo, real_fifo_round_trip_via_fork, .init = ignoreSigpipe)
{
    const std::string p2cPath = uniqueFifoPath("p2c");
    const std::string c2pPath = uniqueFifoPath("c2p");
    NamedPipeChannel::createFifo(p2cPath);
    NamedPipeChannel::createFifo(c2pPath);

    const pid_t pid = ::fork();
    cr_assert_geq(pid, 0);

    if (pid == 0) {
        try {
            const int rfd = NamedPipeChannel::openFifoRead(p2cPath);
            const int wfd = NamedPipeChannel::openFifoWrite(c2pPath);
            NamedPipeChannel child(rfd, wfd);

            Deserializer in{std::vector<std::uint8_t>{}};
            child >> in;
            const Message decoded = decode(in);
            const auto &order = std::get<OrderMsg>(decoded);

            Serializer out;
            encode(out, Message{PizzaDoneMsg{order.orderId, 0}});
            child << out;
        } catch (...) {
            std::_Exit(1);
        }
        std::_Exit(0);
    }

    const int wfd = NamedPipeChannel::openFifoWrite(p2cPath);
    const int rfd = NamedPipeChannel::openFifoRead(c2pPath);
    NamedPipeChannel parent(rfd, wfd);

    Serializer out;
    encode(out, Message{OrderMsg{99, PizzaType::Americana, PizzaSize::S}});
    parent << out;

    Deserializer in{std::vector<std::uint8_t>{}};
    parent >> in;
    const Message decoded = decode(in);
    const auto &done = std::get<PizzaDoneMsg>(decoded);
    cr_assert_eq(done.orderId, 99u);

    int status = 0;
    cr_assert_eq(::waitpid(pid, &status, 0), pid);
    cr_assert(WIFEXITED(status));
    cr_assert_eq(WEXITSTATUS(status), 0);

    parent.close();
    NamedPipeChannel::unlinkFifo(p2cPath);
    NamedPipeChannel::unlinkFifo(c2pPath);
}

Test(named_pipe_state, move_constructor_transfers_ownership)
{
    auto first = makeLoopback();
    const int rfd = first.readFd();
    const int wfd = first.writeFd();

    NamedPipeChannel second(std::move(first));
    cr_assert_eq(second.readFd(), rfd);
    cr_assert_eq(second.writeFd(), wfd);
    cr_assert_not(first.isOpen());
}
