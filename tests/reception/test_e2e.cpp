#include <criterion/criterion.h>

#include "concurrency/Mutex.hpp"
#include "kitchen/Kitchen.hpp"
#include "reception/Reception.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using plazza::concurrency::Mutex;
using plazza::kitchen::KitchenConfig;
using plazza::reception::Reception;
using plazza::reception::ReceptionConfig;
using namespace std::chrono_literals;

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

    std::size_t count(const std::string &needle)
    {
        std::lock_guard<Mutex> lock(mutex);
        std::size_t n = 0;
        for (const auto &line : lines)
            if (line.find(needle) != std::string::npos)
                ++n;
        return n;
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

bool waitUntil(std::function<bool()> predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate())
            return true;
        std::this_thread::sleep_for(10ms);
    }
    return predicate();
}

}

Test(reception_e2e, processes_a_single_order_and_logs_pizza_done, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("margarita M x1\n");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert(log.contains("spawned kitchen"));
    cr_assert(log.contains("dispatched 1"));
    cr_assert(log.contains("pizza done"));
    cr_assert_eq(reception.totalDispatched(), 1u);
    cr_assert_eq(reception.totalCompleted(), 1u);
}

Test(reception_e2e, multiple_orders_are_all_completed, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("margarita S x4\nregina M x2\n");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert_eq(reception.totalDispatched(), 6u);
    cr_assert(waitUntil([&] { return reception.totalCompleted() == 6u; }, 5s));
}

Test(reception_e2e, status_command_logs_status, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("margarita M x1\nstatus\n");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert(log.contains("status:"));
}

Test(reception_e2e, empty_input_exits_cleanly, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert(log.contains("reception stopped"));
    cr_assert_eq(reception.kitchenCount(), 0u);
}

Test(reception_e2e, garbage_lines_are_routed_to_error_log, .timeout = 10)
{
    LogCollector log;
    std::stringstream input("nonsense\nmargarita M x1\n");
    Reception reception(fastConfig(), [&](const std::string &m) { log.append(m); });

    reception.run(input);

    cr_assert(log.contains("parse error"));
    cr_assert(log.contains("pizza done"));
}
