#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "parser/Parser.hpp"
#include "pizza/Types.hpp"
#include "reception/Shell.hpp"

#include <atomic>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using plazza::Error;
using plazza::parser::ParsedOrder;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;
using plazza::reception::Shell;
using plazza::reception::ShellCallbacks;
using namespace std::chrono_literals;

namespace {

struct Collector {
    std::vector<ParsedOrder> orders;
    std::vector<std::string> errors;
    int statusCalls = 0;
};

ShellCallbacks bindCollector(Collector &sink)
{
    ShellCallbacks cb;
    cb.onOrder  = [&sink](const ParsedOrder &order) { sink.orders.push_back(order); };
    cb.onStatus = [&sink] { ++sink.statusCalls; };
    cb.onError  = [&sink](const std::string &what) { sink.errors.push_back(what); };
    return cb;
}

}

Test(shell_construction, missing_order_callback_throws)
{
    std::stringstream input;
    ShellCallbacks cb;
    cr_assert_throw(Shell(input, cb), Error);
}

Test(shell_process_line, blank_line_is_ignored)
{
    Collector sink;
    Shell::processLine("", bindCollector(sink));
    Shell::processLine("   ", bindCollector(sink));
    Shell::processLine("\t  \t", bindCollector(sink));
    cr_assert_eq(sink.orders.size(), 0u);
    cr_assert_eq(sink.errors.size(), 0u);
    cr_assert_eq(sink.statusCalls, 0);
}

Test(shell_process_line, status_command_calls_status_callback)
{
    Collector sink;
    Shell::processLine("status", bindCollector(sink));
    Shell::processLine("STATUS", bindCollector(sink));
    Shell::processLine("  status  ", bindCollector(sink));
    cr_assert_eq(sink.statusCalls, 3);
    cr_assert_eq(sink.orders.size(), 0u);
}

Test(shell_process_line, valid_order_calls_onOrder)
{
    Collector sink;
    Shell::processLine("margarita M x2", bindCollector(sink));
    cr_assert_eq(sink.orders.size(), 1u);
    cr_assert_eq(sink.orders[0].type, PizzaType::Margarita);
    cr_assert_eq(sink.orders[0].size, PizzaSize::M);
    cr_assert_eq(sink.orders[0].count, 2);
}

Test(shell_process_line, multiple_orders_split_by_semicolon)
{
    Collector sink;
    Shell::processLine("margarita M x1; regina L x3", bindCollector(sink));
    cr_assert_eq(sink.orders.size(), 2u);
    cr_assert_eq(sink.orders[1].type, PizzaType::Regina);
    cr_assert_eq(sink.orders[1].size, PizzaSize::L);
    cr_assert_eq(sink.orders[1].count, 3);
}

Test(shell_process_line, parse_error_is_routed_to_onError)
{
    Collector sink;
    Shell::processLine("not a real pizza", bindCollector(sink));
    cr_assert_eq(sink.orders.size(), 0u);
    cr_assert_geq(sink.errors.size(), 1u);
}

Test(shell_process_line, parse_error_with_no_callback_is_swallowed)
{
    ShellCallbacks cb;
    cb.onOrder = [](const ParsedOrder &) {};
    cr_assert_no_throw(Shell::processLine("garbage", cb));
}

Test(shell_run, run_until_eof_processes_all_lines, .timeout = 5)
{
    Collector sink;
    std::stringstream input("margarita M x1\nregina L x2\nstatus\n");
    Shell shell(input, bindCollector(sink));
    shell.runUntilEof();

    cr_assert(shell.finished());
    cr_assert_eq(sink.orders.size(), 2u);
    cr_assert_eq(sink.statusCalls, 1);
}

Test(shell_run, started_thread_processes_lines_and_finishes_on_eof, .timeout = 5)
{
    Collector sink;
    std::stringstream input("margarita M x1\nregina L x1\n");
    Shell shell(input, bindCollector(sink));

    shell.start();
    shell.join();

    cr_assert(shell.finished());
    cr_assert_not(shell.running());
    cr_assert_eq(sink.orders.size(), 2u);
}

Test(shell_run, double_start_throws, .timeout = 5)
{
    Collector sink;
    std::stringstream input("status\n");
    Shell shell(input, bindCollector(sink));

    shell.start();
    cr_assert_throw(shell.start(), Error);
    shell.join();
}

Test(shell_run, mixed_valid_and_invalid_lines, .timeout = 5)
{
    Collector sink;
    std::stringstream input(
        "margarita M x1\n"
        "garbage line\n"
        "regina L x1\n"
        "status\n");
    Shell shell(input, bindCollector(sink));
    shell.runUntilEof();

    cr_assert_eq(sink.orders.size(), 2u);
    cr_assert_eq(sink.statusCalls, 1);
    cr_assert_geq(sink.errors.size(), 1u);
}

Test(shell_run, destructor_joins_running_thread, .timeout = 5)
{
    Collector sink;
    std::stringstream input("status\n");
    {
        Shell shell(input, bindCollector(sink));
        shell.start();
    }
    cr_assert_eq(sink.statusCalls, 1);
}
