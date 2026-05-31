#include <criterion/criterion.h>

#include "reception/Reception.hpp"
#include "reception/SignalHandler.hpp"

#include <csignal>
#include <chrono>
#include <sstream>
#include <thread>

using plazza::kitchen::KitchenConfig;
using plazza::reception::Reception;
using plazza::reception::ReceptionConfig;
using plazza::reception::SignalHandler;
using namespace std::chrono_literals;

namespace {

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

Test(signal_handler, install_then_raise_sets_flag, .timeout = 5)
{
    SignalHandler::reset();
    SignalHandler::install();
    cr_assert_not(SignalHandler::requested());

    std::raise(SIGINT);
    std::this_thread::sleep_for(20ms);
    cr_assert(SignalHandler::requested());

    SignalHandler::reset();
    cr_assert_not(SignalHandler::requested());
    SignalHandler::uninstall();
}

Test(signal_handler, install_is_idempotent)
{
    SignalHandler::install();
    SignalHandler::install();
    SignalHandler::uninstall();
    SignalHandler::uninstall();
}

Test(reception_shutdown, eof_stops_reception_cleanly, .timeout = 10)
{
    std::stringstream input("");
    Reception reception(fastConfig());
    reception.run(input);

    cr_assert_eq(reception.kitchenCount(), 0u);
}

Test(reception_shutdown, requestShutdown_stops_running_reception, .timeout = 10)
{
    // Input that produces orders but no EOF (we'll force a shutdown).
    std::stringstream input("margarita M x1\n");
    Reception reception(fastConfig());

    std::thread runner([&] { reception.run(input); });
    std::this_thread::sleep_for(200ms);
    reception.requestShutdown();
    runner.join();
}

Test(reception_shutdown, destructor_cleans_up_kitchens, .timeout = 10)
{
    std::stringstream input("margarita M x1\n");
    {
        Reception reception(fastConfig());
        reception.run(input);
    }
}
