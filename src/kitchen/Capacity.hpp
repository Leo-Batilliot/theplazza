#pragma once

#include "concurrency/Mutex.hpp"

#include <cstddef>

namespace plazza::kitchen {

class Capacity {
public:
    explicit Capacity(std::size_t maxConcurrent);
    ~Capacity() = default;

    Capacity(const Capacity &) = delete;
    Capacity &operator=(const Capacity &) = delete;
    Capacity(Capacity &&) = delete;
    Capacity &operator=(Capacity &&) = delete;

    [[nodiscard]] bool tryAcquire();
    void release();

    [[nodiscard]] std::size_t inUse() const;
    [[nodiscard]] std::size_t free() const;
    [[nodiscard]] std::size_t max() const noexcept { return _max; }
    [[nodiscard]] bool full() const;

private:
    mutable concurrency::Mutex _mutex;
    std::size_t _max;
    std::size_t _inUse;
};

}
