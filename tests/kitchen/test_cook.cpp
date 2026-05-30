#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "kitchen/Cook.hpp"
#include "kitchen/Restocker.hpp"
#include "kitchen/Stock.hpp"
#include "pizza/Margarita.hpp"
#include "pizza/Regina.hpp"
#include "pizza/Types.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using plazza::KitchenError;
using plazza::kitchen::Cook;
using plazza::kitchen::Restocker;
using plazza::kitchen::Stock;
using plazza::pizza::Ingredient;
using plazza::pizza::Margarita;
using plazza::pizza::PizzaSize;
using plazza::pizza::Regina;
using namespace std::chrono_literals;

Test(cook_basic, cook_consumes_ingredients_and_succeeds, .timeout = 5)
{
    Stock stock;
    Cook cook(stock, 0.05);
    Margarita pizza(PizzaSize::M);

    cr_assert(cook.cook(pizza));
    cr_assert_eq(stock.available(Ingredient::Dough), 4u);
    cr_assert_eq(stock.available(Ingredient::Tomato), 4u);
    cr_assert_eq(stock.available(Ingredient::Gruyere), 4u);
    cr_assert_eq(stock.available(Ingredient::Ham), 5u);
}

Test(cook_basic, cook_duration_scales_with_multiplier)
{
    Stock stock;
    Cook fast(stock, 0.1);
    Cook full(stock, 1.0);
    Margarita pizza(PizzaSize::M);

    cr_assert_eq(full.cookDuration(pizza), pizza.baseCookTime());
    cr_assert_eq(fast.cookDuration(pizza).count(),
        static_cast<long long>(static_cast<double>(pizza.baseCookTime().count()) * 0.1));
}

Test(cook_basic, invalid_multiplier_falls_back_to_default)
{
    Stock stock;
    Cook cook(stock, -3.0);

    cr_assert_eq(cook.timeMultiplier(), Cook::defaultTimeMultiplier);
}

Test(cook_basic, cook_takes_at_least_the_advertised_time, .timeout = 5)
{
    Stock stock;
    Cook cook(stock, 0.1);
    Margarita pizza(PizzaSize::M);
    const auto expected = cook.cookDuration(pizza);

    const auto begin = std::chrono::steady_clock::now();
    cr_assert(cook.cook(pizza));
    const auto elapsed = std::chrono::steady_clock::now() - begin;

    cr_assert_geq(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
        expected.count());
}

Test(cook_blocking, cook_blocks_when_stock_empty_until_restock, .timeout = 5)
{
    Stock stock(0);
    Cook cook(stock, 0.05);
    Margarita pizza(PizzaSize::M);
    std::atomic<bool> done{false};

    std::thread worker([&] {
        done.store(cook.cook(pizza));
    });

    std::this_thread::sleep_for(80ms);
    cr_assert_not(done.load(), "cook must wait for ingredients");

    stock.restockAll(1);
    worker.join();
    cr_assert(done.load());
}

Test(cook_shutdown, cook_returns_false_when_stock_shutdown_while_waiting, .timeout = 5)
{
    Stock stock(0);
    Cook cook(stock, 0.05);
    Margarita pizza(PizzaSize::M);
    std::atomic<int> result{-1};

    std::thread worker([&] {
        result.store(cook.cook(pizza) ? 1 : 0);
    });

    std::this_thread::sleep_for(50ms);
    stock.shutdown();
    worker.join();

    cr_assert_eq(result.load(), 0);
}

Test(cook_parallel, several_cooks_share_a_stock_without_oversubscription, .timeout = 10)
{
    constexpr int total = 40;
    Stock stock(total);
    Cook cook(stock, 0.0);
    Margarita pizza(PizzaSize::M);
    std::atomic<int> ok{0};

    constexpr int workers = 8;
    std::vector<std::thread> threads;
    threads.reserve(workers);
    for (int i = 0; i < workers; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < total / workers; ++j)
                if (cook.cook(pizza))
                    ok.fetch_add(1);
        });
    }
    for (auto &t : threads)
        t.join();

    cr_assert_eq(ok.load(), total);
    cr_assert_eq(stock.available(Ingredient::Dough), 0u);
}

Test(restocker_basic, period_must_be_positive)
{
    Stock stock;
    cr_assert_throw(Restocker(stock, 0ms), KitchenError);
    cr_assert_throw(Restocker(stock, std::chrono::milliseconds(-5)), KitchenError);
}

Test(restocker_basic, starting_twice_throws, .timeout = 5)
{
    Stock stock(0);
    Restocker restocker(stock, 50ms);

    restocker.start();
    cr_assert(restocker.running());
    cr_assert_throw(restocker.start(), KitchenError);
    restocker.stop();
}

Test(restocker_basic, stop_is_idempotent, .timeout = 5)
{
    Stock stock(0);
    Restocker restocker(stock, 50ms);
    restocker.stop();
    restocker.start();
    restocker.stop();
    restocker.stop();

    cr_assert_not(restocker.running());
}

Test(restocker_replenish, periodic_restock_increases_counts, .timeout = 5)
{
    Stock stock(0);
    Restocker restocker(stock, 10ms);

    restocker.start();
    std::this_thread::sleep_for(80ms);
    restocker.stop();

    const auto snap = stock.snapshot();
    for (const auto v : snap)
        cr_assert_geq(v, 1u);
}

Test(restocker_replenish, unblocks_a_cook_waiting_on_empty_stock, .timeout = 5)
{
    Stock stock(0);
    Restocker restocker(stock, 10ms);
    Cook cook(stock, 0.0);
    Regina pizza(PizzaSize::M);
    std::atomic<bool> done{false};

    std::thread worker([&] {
        done.store(cook.cook(pizza));
    });

    restocker.start();
    worker.join();
    restocker.stop();

    cr_assert(done.load());
}

Test(restocker_shutdown, destructor_stops_the_thread, .timeout = 5)
{
    Stock stock(0);
    {
        Restocker restocker(stock, 10ms);
        restocker.start();
        std::this_thread::sleep_for(40ms);
    }
    const auto a = stock.available(Ingredient::Dough);
    std::this_thread::sleep_for(50ms);
    cr_assert_eq(stock.available(Ingredient::Dough), a,
        "no more restocks should happen after destruction");
}
