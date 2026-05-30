#pragma once

#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"
#include "concurrency/Thread.hpp"

#include <chrono>
#include <cstdint>

namespace plazza::kitchen {

class Stock;

class Restocker {
public:
    static constexpr std::uint32_t defaultAmount = 1;

    Restocker(Stock &stock, std::chrono::milliseconds period,
        std::uint32_t amount = defaultAmount);
    ~Restocker();

    Restocker(const Restocker &) = delete;
    Restocker &operator=(const Restocker &) = delete;
    Restocker(Restocker &&) = delete;
    Restocker &operator=(Restocker &&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool running() const;
    [[nodiscard]] std::chrono::milliseconds period() const noexcept { return _period; }
    [[nodiscard]] std::uint32_t amount() const noexcept { return _amount; }

private:
    void loop();

    Stock &_stock;
    std::chrono::milliseconds _period;
    std::uint32_t _amount;

    mutable concurrency::Mutex _mutex;
    concurrency::ConditionVar _condition;
    concurrency::Thread _thread;
    bool _running;
    bool _stopRequested;
};

}
