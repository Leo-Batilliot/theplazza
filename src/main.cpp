#include "cli/Args.hpp"
#include "common/Errors.hpp"
#include "logging/Logger.hpp"
#include "reception/Reception.hpp"

#include <fstream>
#include <iostream>

namespace {

constexpr int ExitCode = 84;

const char *progName(char **argv)
{
    return (argv && argv[0] && *argv[0]) ? argv[0] : "plazza";
}

plazza::reception::ReceptionConfig configFromArgs(const plazza::cli::Args &args)
{
    plazza::reception::ReceptionConfig cfg;
    cfg.kitchenDefaults.cookCount      = args.cooksPerKitchen();
    cfg.kitchenDefaults.timeMultiplier = args.multiplier();
    cfg.kitchenDefaults.restockPeriod  = args.restockTime();
    cfg.kitchenDefaults.idleTimeout    = std::chrono::milliseconds(5000);
    cfg.installSignalHandler           = true;
    return cfg;
}

}

int main(int argc, char **argv)
{
    try {
        const auto args = plazza::cli::Args::fromArgv(argc, argv);
        plazza::logging::Logger logger("plazza.log");
        auto cfg = configFromArgs(args);
        plazza::reception::Reception reception(std::move(cfg),
            [&logger](const std::string &msg) { logger.log(msg); });
        reception.run(std::cin);
        return 0;
    } catch (const plazza::CliError &e) {
        std::cerr << "Plazza: " << e.what() << "\n" << plazza::cli::Args::usage(progName(argv)) << std::endl;
        return ExitCode;
    } catch (const plazza::Error &e) {
        std::cerr << "Plazza: fatal: " << e.what() << std::endl;
        return ExitCode;
    } catch (const std::exception &e) {
        std::cerr << "Plazza: unexpected: " << e.what() << std::endl;
        return ExitCode;
    }
}
