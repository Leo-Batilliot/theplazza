#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "kitchen/Stock.hpp"
#include "pizza/Types.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using plazza::StockError;
using plazza::kitchen::Stock;
using plazza::pizza::Ingredient;
using plazza::pizza::ingredientCount;
using namespace std::chrono_literals;

namespace {

std::vector<Ingredient> margaritaIngredients()
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere};
}

std::vector<Ingredient> reginaIngredients()
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere,
        Ingredient::Ham, Ingredient::Mushrooms};
}

}

Test(stock_init, default_initialises_every_ingredient_to_five)
{
    Stock stock;

    for (std::uint32_t i = 0; i < ingredientCount; ++i)
        cr_assert_eq(stock.available(static_cast<Ingredient>(i)), Stock::initialCount);
}

Test(stock_init, explicit_initial_value_is_applied)
{
    Stock stock(42);

    const auto snap = stock.snapshot();
    for (const auto v : snap)
        cr_assert_eq(v, 42u);
}

Test(stock_consume, try_consume_succeeds_when_enough)
{
    Stock stock;

    cr_assert(stock.tryConsume(margaritaIngredients()));
    cr_assert_eq(stock.available(Ingredient::Dough), 4u);
    cr_assert_eq(stock.available(Ingredient::Tomato), 4u);
    cr_assert_eq(stock.available(Ingredient::Gruyere), 4u);
    cr_assert_eq(stock.available(Ingredient::Ham), 5u);
}

Test(stock_consume, try_consume_is_atomic_partial_does_not_decrement)
{
    Stock stock(1);

    const std::vector<Ingredient> need{Ingredient::Dough, Ingredient::Tomato, Ingredient::Dough};
    cr_assert_not(stock.tryConsume(need));
    cr_assert_eq(stock.available(Ingredient::Dough), 1u);
    cr_assert_eq(stock.available(Ingredient::Tomato), 1u);
}

Test(stock_consume, try_consume_handles_duplicate_ingredients)
{
    Stock stock(2);

    const std::vector<Ingredient> need{Ingredient::Dough, Ingredient::Dough};
    cr_assert(stock.tryConsume(need));
    cr_assert_eq(stock.available(Ingredient::Dough), 0u);
}

Test(stock_consume, try_consume_drains_to_zero_then_fails)
{
    Stock stock;

    for (int i = 0; i < 5; ++i)
        cr_assert(stock.tryConsume(margaritaIngredients()));
    cr_assert_not(stock.tryConsume(margaritaIngredients()));
    cr_assert_eq(stock.available(Ingredient::Dough), 0u);
}

Test(stock_consume, empty_ingredient_list_succeeds_without_change)
{
    Stock stock;
    const auto before = stock.snapshot();

    cr_assert(stock.tryConsume({}));
    const auto after = stock.snapshot();
    for (std::size_t i = 0; i < before.size(); ++i)
        cr_assert_eq(before[i], after[i]);
}

Test(stock_restock, restock_single_ingredient_increments_only_that_slot)
{
    Stock stock(0);

    stock.restock(Ingredient::Ham, 3);
    cr_assert_eq(stock.available(Ingredient::Ham), 3u);
    cr_assert_eq(stock.available(Ingredient::Dough), 0u);
}

Test(stock_restock, restock_all_increments_every_slot_by_one)
{
    Stock stock(0);

    stock.restockAll();
    for (const auto v : stock.snapshot())
        cr_assert_eq(v, 1u);
}

Test(stock_restock, restock_zero_is_noop)
{
    Stock stock(7);
    stock.restock(Ingredient::Dough, 0);
    stock.restockAll(0);
    for (const auto v : stock.snapshot())
        cr_assert_eq(v, 7u);
}

Test(stock_wait, wait_and_consume_succeeds_immediately_when_enough)
{
    Stock stock;

    cr_assert(stock.waitAndConsume(margaritaIngredients()));
    cr_assert_eq(stock.available(Ingredient::Dough), 4u);
}

Test(stock_wait, wait_and_consume_blocks_until_restock, .timeout = 5)
{
    Stock stock(0);
    std::atomic<bool> consumed{false};

    std::thread waiter([&] {
        const bool ok = stock.waitAndConsume(margaritaIngredients());
        consumed.store(ok);
    });

    std::this_thread::sleep_for(100ms);
    cr_assert_not(consumed.load(), "waiter must still be blocked while stock is empty");

    stock.restockAll(1);
    waiter.join();

    cr_assert(consumed.load());
    cr_assert_eq(stock.available(Ingredient::Dough), 0u);
    cr_assert_eq(stock.available(Ingredient::Tomato), 0u);
    cr_assert_eq(stock.available(Ingredient::Gruyere), 0u);
    cr_assert_eq(stock.available(Ingredient::Ham), 1u);
}

Test(stock_wait, wait_and_consume_only_wakes_when_all_needed_ingredients_available, .timeout = 5)
{
    Stock stock(0);
    std::atomic<bool> consumed{false};

    std::thread waiter([&] {
        consumed.store(stock.waitAndConsume(reginaIngredients()));
    });

    std::this_thread::sleep_for(50ms);
    stock.restock(Ingredient::Dough, 1);
    stock.restock(Ingredient::Tomato, 1);
    stock.restock(Ingredient::Gruyere, 1);
    stock.restock(Ingredient::Ham, 1);
    std::this_thread::sleep_for(100ms);
    cr_assert_not(consumed.load(), "waiter must wait until mushrooms arrive too");

    stock.restock(Ingredient::Mushrooms, 1);
    waiter.join();
    cr_assert(consumed.load());
}

Test(stock_shutdown, shutdown_unblocks_waiter_returning_false, .timeout = 5)
{
    Stock stock(0);
    std::atomic<bool> consumed{true};
    std::atomic<bool> finished{false};

    std::thread waiter([&] {
        consumed.store(stock.waitAndConsume(margaritaIngredients()));
        finished.store(true);
    });

    std::this_thread::sleep_for(50ms);
    cr_assert_not(finished.load());

    stock.shutdown();
    waiter.join();

    cr_assert(finished.load());
    cr_assert_not(consumed.load());
    cr_assert(stock.isShutdown());
}

Test(stock_shutdown, try_consume_returns_false_after_shutdown)
{
    Stock stock;
    stock.shutdown();

    cr_assert_not(stock.tryConsume(margaritaIngredients()));
    cr_assert_eq(stock.available(Ingredient::Dough), Stock::initialCount);
}

Test(stock_shutdown, shutdown_wakes_multiple_waiters, .timeout = 5)
{
    Stock stock(0);
    constexpr int waiters = 4;
    std::atomic<int> finished{0};

    std::vector<std::thread> threads;
    threads.reserve(waiters);
    for (int i = 0; i < waiters; ++i) {
        threads.emplace_back([&] {
            (void) stock.waitAndConsume(margaritaIngredients());
            finished.fetch_add(1);
        });
    }

    std::this_thread::sleep_for(50ms);
    stock.shutdown();
    for (auto &t : threads)
        t.join();

    cr_assert_eq(finished.load(), waiters);
}

Test(stock_snapshot, snapshot_returns_independent_copy)
{
    Stock stock;
    auto first = stock.snapshot();
    (void) stock.tryConsume(margaritaIngredients());
    auto second = stock.snapshot();

    cr_assert_eq(first[static_cast<std::size_t>(Ingredient::Dough)], 5u);
    cr_assert_eq(second[static_cast<std::size_t>(Ingredient::Dough)], 4u);
}

Test(stock_concurrent, parallel_consumers_never_oversubscribe, .timeout = 10)
{
    constexpr int totalUnits = 200;
    Stock stock(totalUnits);
    constexpr int consumers = 8;
    constexpr int perThread = 20;
    std::atomic<int> successes{0};

    std::vector<std::thread> threads;
    threads.reserve(consumers);
    for (int t = 0; t < consumers; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < perThread; ++i) {
                if (stock.tryConsume(margaritaIngredients()))
                    successes.fetch_add(1);
            }
        });
    }
    for (auto &t : threads)
        t.join();

    const int doughLeft = static_cast<int>(stock.available(Ingredient::Dough));
    cr_assert_eq(successes.load() + doughLeft, totalUnits,
        "every consume must show in counters: %d consumed + %d remaining",
        successes.load(), doughLeft);
    cr_assert_geq(doughLeft, 0);
}

Test(stock_concurrent, restocker_thread_drives_consumers_to_completion, .timeout = 10)
{
    Stock stock(0);
    constexpr int consumers = 4;
    constexpr int perThread = 10;
    std::atomic<int> consumed{0};
    std::atomic<bool> stopRestocker{false};

    std::thread restocker([&] {
        while (!stopRestocker.load()) {
            stock.restockAll(1);
            std::this_thread::sleep_for(1ms);
        }
    });

    std::vector<std::thread> threads;
    threads.reserve(consumers);
    for (int t = 0; t < consumers; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < perThread; ++i) {
                if (stock.waitAndConsume(reginaIngredients()))
                    consumed.fetch_add(1);
            }
        });
    }
    for (auto &t : threads)
        t.join();

    stopRestocker.store(true);
    restocker.join();

    cr_assert_eq(consumed.load(), consumers * perThread);
}
