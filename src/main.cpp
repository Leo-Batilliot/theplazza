#include "cli/Args.hpp"
#include "common/Errors.hpp"

#include <iostream>

namespace {

constexpr int ExitCode = 84;

const char *progName(char **argv)
{
    return (argv && argv[0] && *argv[0]) ? argv[0] : "plazza";
}

}

int main(int argc, char **argv)
{
    try {
        const auto args = plazza::cli::Args::fromArgv(argc, argv);
        (void)args;
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
