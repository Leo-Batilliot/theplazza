#include <criterion/criterion.h>

#include "reception/Reception.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

using plazza::reception::Reception;
using plazza::reception::ReceptionConfig;
using namespace std::chrono_literals;

namespace {

ReceptionConfig stressConfig()
{
    ReceptionConfig cfg;
    cfg.kitchenDefaults.cookCount       = 4;
    cfg.kitchenDefaults.timeMultiplier  = 0.005;
    cfg.kitchenDefaults.restockPeriod   = 10ms;
    cfg.kitchenDefaults.idleTimeout     = 10000ms;
    cfg.kitchenDefaults.stockInitial    = 200;
    cfg.pollInterval                    = 20ms;
    cfg.drainTimeout                    = 60000ms;
    cfg.installSignalHandler            = false;
    return cfg;
}

std::string burstScript(std::size_t totalPizzas)
{
    std::ostringstream oss;
    const char *types[] = {"margarita", "regina", "americana", "fantasia"};
    const char *sizes[] = {"S", "M", "L", "XL", "XXL"};
    std::size_t remaining = totalPizzas;
    std::size_t i = 0;
    while (remaining > 0) {
        const std::size_t batch = remaining < 25 ? remaining : 25;
        oss << types[i % 4] << " " << sizes[i % 5] << " x" << batch << "\n";
        remaining -= batch;
        ++i;
    }
    return oss.str();
}

}

Test(stress, thousand_pizzas_drain_within_budget, .timeout = 60)
{
    constexpr std::size_t totalPizzas = 1000;
    std::stringstream input(burstScript(totalPizzas));
    Reception reception(stressConfig());

    const auto begin = std::chrono::steady_clock::now();
    reception.run(input);
    const auto elapsed = std::chrono::steady_clock::now() - begin;

    cr_assert_eq(reception.totalDispatched(), totalPizzas,
        "dispatched %lu, expected %zu",
        static_cast<unsigned long>(reception.totalDispatched()), totalPizzas);
    cr_assert_eq(reception.totalCompleted(), totalPizzas,
        "completed %lu, expected %zu",
        static_cast<unsigned long>(reception.totalCompleted()), totalPizzas);

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    cr_assert_lt(seconds, 50);
}

Test(stress, two_hundred_pizzas_with_small_kitchens, .timeout = 30)
{
    constexpr std::size_t totalPizzas = 200;
    ReceptionConfig cfg = stressConfig();
    cfg.kitchenDefaults.cookCount    = 2;
    cfg.kitchenDefaults.stockInitial = 80;

    std::stringstream input(burstScript(totalPizzas));
    Reception reception(std::move(cfg));
    reception.run(input);

    cr_assert_eq(reception.totalCompleted(), totalPizzas);
}
