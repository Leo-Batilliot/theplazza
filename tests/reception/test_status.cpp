#include <criterion/criterion.h>

#include "ipc/Messages.hpp"
#include "kitchen/Kitchen.hpp"
#include "reception/Reception.hpp"
#include "reception/StatusCommand.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "concurrency/Mutex.hpp"

using plazza::concurrency::Mutex;
using plazza::ipc::StatusReplyMsg;
using plazza::reception::Reception;
using plazza::reception::ReceptionConfig;
using plazza::reception::StatusCommand;
using namespace std::chrono_literals;

Test(status_command_format, reply_includes_kitchen_id_and_busy_counts)
{
    StatusReplyMsg reply{};
    reply.kitchenId  = 42;
    reply.cooksTotal = 3;
    reply.cooksBusy  = 1;
    reply.queueSize  = 7;
    for (std::size_t i = 0; i < reply.stock.size(); ++i)
        reply.stock[i] = static_cast<std::uint32_t>(i + 1);

    const std::string text = StatusCommand::formatReply(reply);
    cr_assert(text.find("k42") != std::string::npos);
    cr_assert(text.find("cooks=1/3") != std::string::npos);
    cr_assert(text.find("queue=7") != std::string::npos);
    cr_assert(text.find("dough:1") != std::string::npos);
}

Test(status_command_format, handle_line_uses_in_flight_and_max)
{
    const std::string text = StatusCommand::formatHandle(2, 3, 6);
    cr_assert(text.find("k2") != std::string::npos);
    cr_assert(text.find("3/6") != std::string::npos);
}

Test(status_command_format, summary_lists_all_replies)
{
    StatusReplyMsg a{};
    a.kitchenId = 1; a.cooksTotal = 2; a.cooksBusy = 0;
    StatusReplyMsg b{};
    b.kitchenId = 2; b.cooksTotal = 2; b.cooksBusy = 1;
    const std::string text = StatusCommand::formatSummary(2, {a, b});
    cr_assert(text.find("2 kitchens") != std::string::npos);
    cr_assert(text.find("k1") != std::string::npos);
    cr_assert(text.find("k2") != std::string::npos);
}

Test(status_command_format, ingredient_name_clamps_on_overflow)
{
    cr_assert_eq(StatusCommand::ingredientName(0), std::string("dough"));
    cr_assert_eq(StatusCommand::ingredientName(999), std::string("?"));
}

namespace {

struct LogCollector {
    Mutex mutex;
    std::vector<std::string> lines;

    void append(const std::string &line)
    {
        std::lock_guard<Mutex> lock(mutex);
        lines.push_back(line);
    }

    bool contains(const std::string &needle)
    {
        std::lock_guard<Mutex> lock(mutex);
        for (const auto &line : lines)
            if (line.find(needle) != std::string::npos)
                return true;
        return false;
    }
};

ReceptionConfig fastConfig()
{
    ReceptionConfig cfg;
    cfg.kitchenDefaults.cookCount       = 2;
    cfg.kitchenDefaults.timeMultiplier  = 0.01;
    cfg.kitchenDefaults.restockPeriod   = 20ms;
    cfg.kitchenDefaults.idleTimeout     = 3000ms;
    cfg.kitchenDefaults.stockInitial    = 50;
    cfg.pollInterval                    = 20ms;
    cfg.installSignalHandler            = false;
    return cfg;
}

}

Test(reception_status, status_in_shell_produces_handle_lines_and_reply, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("margarita M x1\nstatus\n");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert(log.contains("status: 1 kitchens"));
    cr_assert(log.contains("status-reply"));
    cr_assert(log.contains("cooks="));
}
