#include "concurrency/ConditionVar.hpp"

namespace plazza::concurrency {

void ConditionVar::notify_one() noexcept
{
    _condition.notify_one();
}

void ConditionVar::notify_all() noexcept
{
    _condition.notify_all();
}

}
