#include "reception/SignalHandler.hpp"

#include <csignal>

namespace plazza::reception {

std::atomic<bool> SignalHandler::_requested{false};
std::atomic<bool> SignalHandler::_installed{false};

void SignalHandler::install()
{
    if (_installed.exchange(true))
        return;
    struct sigaction sa{};
    sa.sa_handler = &SignalHandler::onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
}

void SignalHandler::uninstall()
{
    if (!_installed.exchange(false))
        return;
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
}

bool SignalHandler::requested() noexcept
{
    return _requested.load();
}

void SignalHandler::reset() noexcept
{
    _requested.store(false);
}

void SignalHandler::onSignal(int) noexcept
{
    _requested.store(true);
}

}
