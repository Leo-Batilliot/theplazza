#pragma once

#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"
#include "pizza/Types.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace plazza::kitchen {

class Stock {
public:
    static constexpr std::uint32_t initialCount = 5;

    Stock() noexcept;
    explicit Stock(std::uint32_t initial) noexcept;
    ~Stock() = default;

    Stock(const Stock &) = delete;
    Stock &operator=(const Stock &) = delete;
    Stock(Stock &&) = delete;
    Stock &operator=(Stock &&) = delete;

    [[nodiscard]] bool tryConsume(const std::vector<pizza::Ingredient> &ingredients);
    [[nodiscard]] bool waitAndConsume(const std::vector<pizza::Ingredient> &ingredients);

    void restock(pizza::Ingredient ingredient, std::uint32_t amount = 1);
    void restockAll(std::uint32_t amount = 1);

    [[nodiscard]] std::uint32_t available(pizza::Ingredient ingredient) const;
    [[nodiscard]] std::array<std::uint32_t, pizza::ingredientCount> snapshot() const;

    void shutdown();
    [[nodiscard]] bool isShutdown() const;

private:
    using Counts = std::array<std::uint32_t, pizza::ingredientCount>;

    [[nodiscard]] static Counts aggregate(const std::vector<pizza::Ingredient> &ingredients);
    [[nodiscard]] bool hasEnoughLocked(const Counts &required) const noexcept;
    void consumeLocked(const Counts &required) noexcept;

    mutable concurrency::Mutex _mutex;
    concurrency::ConditionVar _condition;
    Counts _counts;
    bool _shutdown;
};

}
