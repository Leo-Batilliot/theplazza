#pragma once

#include <chrono>
#include <condition_variable>

namespace plazza::concurrency {

class ConditionVar {
public:
    ConditionVar() = default;
    ~ConditionVar() = default;

    ConditionVar(const ConditionVar &) = delete;
    ConditionVar &operator=(const ConditionVar &) = delete;
    ConditionVar(ConditionVar &&) = delete;
    ConditionVar &operator=(ConditionVar &&) = delete;

    template <typename Lock>
    void wait(Lock &lock)
    {
        _condition.wait(lock);
    }

    template <typename Lock, typename Predicate>
    void wait(Lock &lock, Predicate predicate)
    {
        _condition.wait(lock, std::move(predicate));
    }

    template <typename Lock, typename Rep, typename Period>
    std::cv_status wait_for(Lock &lock, const std::chrono::duration<Rep, Period> &timeout)
    {
        return _condition.wait_for(lock, timeout);
    }

    template <typename Lock, typename Rep, typename Period, typename Predicate>
    bool wait_for(Lock &lock, const std::chrono::duration<Rep, Period> &timeout, Predicate predicate)
    {
        return _condition.wait_for(lock, timeout, std::move(predicate));
    }

    void notify_one() noexcept;
    void notify_all() noexcept;

private:
    std::condition_variable_any _condition;
};

}
