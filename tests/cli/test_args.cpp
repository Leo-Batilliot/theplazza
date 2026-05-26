#include <criterion/criterion.h>

#include "cli/Args.hpp"
#include "common/Errors.hpp"

#include <initializer_list>
#include <string>
#include <vector>

namespace {

struct Argv {
    std::vector<std::string> storage;
    std::vector<char *> ptrs;

    Argv(std::initializer_list<const char *> args) : storage(args.begin(), args.end())
    {
        ptrs.reserve(storage.size());
        for (auto &s : storage)
            ptrs.push_back(s.data());
    }

    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

#define EXPECT_CLI_ERROR(EXPR)                                                 \
    do {                                                                       \
        bool _thrown = false;                                                  \
        try {                                                                  \
            (void)(EXPR);                                                      \
        } catch (const plazza::CliError &) {                                   \
            _thrown = true;                                                    \
        } catch (...) {                                                        \
            cr_assert_fail("expected plazza::CliError, got other exception");  \
        }                                                                      \
        cr_assert(_thrown, "expected plazza::CliError to be thrown");          \
    } while (0)

}

using plazza::cli::Args;

Test(args_valid, parses_integer_multiplier)
{
    Argv av{"plazza", "2", "4", "1000"};
    Args a = Args::fromArgv(av.argc(), av.argv());

    cr_assert_float_eq(a.multiplier(), 2.0f, 1e-6f);
    cr_assert_eq(a.cooksPerKitchen(), 4u);
    cr_assert_eq(a.restockTime().count(), 1000);
}

Test(args_valid, parses_fractional_multiplier)
{
    Argv av{"plazza", "0.5", "3", "500"};
    Args a = Args::fromArgv(av.argc(), av.argv());

    cr_assert_float_eq(a.multiplier(), 0.5f, 1e-6f);
    cr_assert_eq(a.cooksPerKitchen(), 3u);
    cr_assert_eq(a.restockTime().count(), 500);
}

Test(args_valid, multiplier_zero_is_accepted)
{
    Argv av{"plazza", "0", "1", "1"};
    Args a = Args::fromArgv(av.argc(), av.argv());

    cr_assert_float_eq(a.multiplier(), 0.0f, 1e-6f);
}

Test(args_valid, multiplier_with_leading_dot)
{
    Argv av{"plazza", ".25", "2", "750"};
    Args a = Args::fromArgv(av.argc(), av.argv());

    cr_assert_float_eq(a.multiplier(), 0.25f, 1e-6f);
}

Test(args_valid, large_values)
{
    Argv av{"plazza", "1", "16", "100000"};
    Args a = Args::fromArgv(av.argc(), av.argv());

    cr_assert_eq(a.cooksPerKitchen(), 16u);
    cr_assert_eq(a.restockTime().count(), 100000);
}

Test(args_argc, no_extra_arg_rejected)
{
    Argv av{"plazza"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_argc, one_arg_rejected)
{
    Argv av{"plazza", "1"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_argc, two_args_rejected)
{
    Argv av{"plazza", "1", "2"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_argc, four_args_rejected)
{
    Argv av{"plazza", "1", "2", "3", "4"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_multiplier, negative_rejected)
{
    Argv av{"plazza", "-1", "2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_multiplier, non_numeric_rejected)
{
    Argv av{"plazza", "abc", "2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_multiplier, empty_rejected)
{
    Argv av{"plazza", "", "2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_multiplier, trailing_garbage_rejected)
{
    Argv av{"plazza", "1.5x", "2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_multiplier, out_of_range_rejected)
{
    Argv av{"plazza", "1e400", "2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, zero_rejected)
{
    Argv av{"plazza", "1", "0", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, negative_rejected)
{
    Argv av{"plazza", "1", "-2", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, non_numeric_rejected)
{
    Argv av{"plazza", "1", "two", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, trailing_garbage_rejected)
{
    Argv av{"plazza", "1", "5x", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, empty_rejected)
{
    Argv av{"plazza", "1", "", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_cooks, out_of_range_rejected)
{
    Argv av{"plazza", "1", "99999999999999999999999999", "100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_restock, zero_rejected)
{
    Argv av{"plazza", "1", "2", "0"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_restock, negative_rejected)
{
    Argv av{"plazza", "1", "2", "-100"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_restock, non_numeric_rejected)
{
    Argv av{"plazza", "1", "2", "fast"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_restock, trailing_garbage_rejected)
{
    Argv av{"plazza", "1", "2", "100ms"};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_restock, empty_rejected)
{
    Argv av{"plazza", "1", "2", ""};
    EXPECT_CLI_ERROR(Args::fromArgv(av.argc(), av.argv()));
}

Test(args_usage, mentions_provided_program_name)
{
    const std::string text = Args::usage("./plazza");

    cr_assert_neq(text.find("./plazza"), std::string::npos);
    cr_assert_neq(text.find("multiplier"), std::string::npos);
    cr_assert_neq(text.find("cooks_per_kitchen"), std::string::npos);
    cr_assert_neq(text.find("restock_time_ms"), std::string::npos);
}

Test(args_usage, falls_back_when_program_name_null)
{
    const std::string text = Args::usage(nullptr);

    cr_assert_neq(text.find("Plazza"), std::string::npos);
}

Test(args_usage, falls_back_when_program_name_empty)
{
    const std::string text = Args::usage("");

    cr_assert_neq(text.find("Plazza"), std::string::npos);
}
