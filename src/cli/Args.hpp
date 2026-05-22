#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace plazza::cli {

class Args {
public:
    static Args fromArgv(int argc, char **argv);
    static std::string usage(const char *progName);

    [[nodiscard]] float multiplier() const noexcept { return _multiplier; }
    [[nodiscard]] std::size_t cooksPerKitchen() const noexcept { return _cooksPerKitchen; }
    [[nodiscard]] std::chrono::milliseconds restockTime() const noexcept { return _restockTime; }

private:
    Args(float multiplier, std::size_t cooksPerKitchen, std::chrono::milliseconds restockTime);

    float _multiplier;
    std::size_t _cooksPerKitchen;
    std::chrono::milliseconds _restockTime;
};

}
