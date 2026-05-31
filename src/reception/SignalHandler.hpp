#pragma once

#include <atomic>

namespace plazza::reception {

class SignalHandler {
public:
    static void install();
    static void uninstall();
    static bool requested() noexcept;
    static void reset() noexcept;

private:
    static void onSignal(int signal) noexcept;
    static std::atomic<bool> _requested;
    static std::atomic<bool> _installed;
};

}
