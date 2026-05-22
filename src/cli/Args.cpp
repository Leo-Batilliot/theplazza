#include "cli/Args.hpp"
#include "common/Errors.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

namespace plazza::cli {

namespace {

bool startsWithDigit(const std::string &s)
{
    return !s.empty() && std::isdigit(static_cast<unsigned char>(s.front()));
}

std::size_t parsePositiveInt(const std::string &str, const char *what)
{
    if (!startsWithDigit(str))
        throw CliError(std::string{what} + ": must be a positive integer");
    std::size_t pos = 0;
    unsigned long value = 0;
    try {
        value = std::stoul(str, &pos);
    } catch (const std::out_of_range &) {
        throw CliError(std::string{what} + ": value out of range");
    } catch (const std::exception &) {
        throw CliError(std::string{what} + ": must be a positive integer");
    }
    if (pos != str.size())
        throw CliError(std::string{what} + ": trailing garbage");
    if (value == 0)
        throw CliError(std::string{what} + ": must be > 0");
    return static_cast<std::size_t>(value);
}

float parseMultiplier(const std::string &str)
{
    if (str.empty())
        throw CliError("multiplier: empty value");
    if (const char first = str.front(); !std::isdigit(static_cast<unsigned char>(first)) && first != '.')
        throw CliError("multiplier: must be a non-negative number");
    std::size_t pos = 0;
    float value = 0.0f;
    try {
        value = std::stof(str, &pos);
    } catch (const std::out_of_range &) {
        throw CliError("multiplier: value out of range");
    } catch (const std::exception &) {
        throw CliError("multiplier: must be a non-negative number");
    }
    if (pos != str.size())
        throw CliError("multiplier: trailing garbage");
    if (!(value >= 0.0f))
        throw CliError("multiplier: must be >= 0");
    return value;
}

}

Args::Args(const float multiplier, const std::size_t cooksPerKitchen, const std::chrono::milliseconds restockTime) : _multiplier(multiplier), _cooksPerKitchen(cooksPerKitchen), _restockTime(restockTime) {}

std::string Args::usage(const char *progName)
{
    const std::string name = (progName && *progName) ? progName : "Plazza";
    return "USAGE: " + name + " <multiplier> <cooks_per_kitchen> <restock_time_ms>\n"
        "  multiplier         non-negative float (acts as divisor when in [0,1])\n"
        "  cooks_per_kitchen  positive integer\n"
        "  restock_time_ms    positive integer (ms between +1 of every ingredient)";
}

Args Args::fromArgv(const int argc, char **argv)
{
    if (argc != 4)
        throw CliError("expected 3 arguments, got " + std::to_string(argc - 1));
    return Args{parseMultiplier(argv[1]), parsePositiveInt(argv[2], "cooks_per_kitchen"), std::chrono::milliseconds{parsePositiveInt(argv[3], "restock_time_ms")}};
}

}
