#include <criterion/criterion.h>

#include "common/Errors.hpp"

#include <exception>
#include <string>

Test(errors_base, what_returns_message)
{
    const plazza::Error e("boom");

    cr_assert_str_eq(e.what(), "boom");
}

Test(errors_base, empty_message_is_preserved)
{
    const plazza::Error e("");

    cr_assert_str_eq(e.what(), "");
}

Test(errors_base, inherits_std_exception)
{
    try {
        throw plazza::Error("base");
    } catch (const std::exception &e) {
        cr_assert_str_eq(e.what(), "base");
        return;
    }
    cr_assert_fail("plazza::Error should be catchable as std::exception");
}

Test(errors_hierarchy, cli_error_is_plazza_error)
{
    try {
        throw plazza::CliError("bad arg");
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "bad arg");
        return;
    }
    cr_assert_fail("CliError should be catchable as plazza::Error");
}

Test(errors_hierarchy, parse_error_is_plazza_error)
{
    try {
        throw plazza::ParseError("bad token");
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "bad token");
        return;
    }
    cr_assert_fail("ParseError should be catchable as plazza::Error");
}

Test(errors_hierarchy, kitchen_error_is_plazza_error)
{
    try {
        throw plazza::KitchenError("kitchen down");
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "kitchen down");
        return;
    }
    cr_assert_fail("KitchenError should be catchable as plazza::Error");
}

Test(errors_hierarchy, stock_error_is_plazza_error)
{
    try {
        throw plazza::StockError("out of dough");
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "out of dough");
        return;
    }
    cr_assert_fail("StockError should be catchable as plazza::Error");
}

Test(errors_hierarchy, process_error_is_plazza_error)
{
    try {
        throw plazza::ProcessError("fork failed");
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "fork failed");
        return;
    }
    cr_assert_fail("ProcessError should be catchable as plazza::Error");
}

Test(errors_hierarchy, ipc_error_is_plazza_error)
{
    try {
        throw plazza::IpcError("pipe closed", 32);
    } catch (const plazza::Error &e) {
        cr_assert_str_eq(e.what(), "pipe closed");
        return;
    }
    cr_assert_fail("IpcError should be catchable as plazza::Error");
}

Test(errors_ipc, captures_errno_snapshot)
{
    const plazza::IpcError e("read failed", 11);

    cr_assert_str_eq(e.what(), "read failed");
    cr_assert_eq(e.errnoSnapshot(), 11);
}

Test(errors_ipc, accepts_zero_errno)
{
    const plazza::IpcError e("clean close", 0);

    cr_assert_eq(e.errnoSnapshot(), 0);
}

Test(errors_subclasses, do_not_swallow_each_other)
{
    bool caught = false;

    try {
        throw plazza::ParseError("token");
    } catch (const plazza::CliError &) {
        cr_assert_fail("ParseError must not match CliError catch");
    } catch (const plazza::Error &) {
        caught = true;
    }
    cr_assert(caught, "ParseError must still be caught as plazza::Error");
}
