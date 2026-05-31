#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "logging/Logger.hpp"

#include <unistd.h>

#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using plazza::logging::Level;
using plazza::logging::Logger;

namespace {

std::string randomPath(const char *suffix)
{
    static int counter = 0;
    return std::string("/tmp/plazza-log-") + suffix + "-" +
        std::to_string(::getpid()) + "-" + std::to_string(++counter) + ".log";
}

}

Test(logger_basic, writes_a_line_to_sink)
{
    std::ostringstream out;
    Logger logger(out, false);
    logger.log("hello");

    const std::string contents = out.str();
    cr_assert(contents.find("[INFO]") != std::string::npos);
    cr_assert(contents.find("hello") != std::string::npos);
    cr_assert(contents.back() == '\n');
}

Test(logger_basic, supports_levels)
{
    std::ostringstream out;
    Logger logger(out, false);
    logger.log(Level::Warn, "careful");
    logger.log(Level::Error, "doom");

    const std::string contents = out.str();
    cr_assert(contents.find("[WARN] careful") != std::string::npos);
    cr_assert(contents.find("[ERROR] doom") != std::string::npos);
}

Test(logger_basic, after_close_no_writes)
{
    std::ostringstream out;
    Logger logger(out, false);
    logger.close();
    logger.log("ignored");

    cr_assert_eq(out.str().size(), 0u);
}

Test(logger_basic, opening_unwritable_file_throws)
{
    cr_assert_throw(Logger("/proc/this-cannot-be-written/plazza.log"), plazza::Error);
}

Test(logger_basic, file_sink_persists_lines)
{
    const std::string path = randomPath("file");
    {
        Logger logger(path, false);
        logger.log("first");
        logger.log("second");
    }

    std::ifstream in(path);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    cr_assert(contents.find("first") != std::string::npos);
    cr_assert(contents.find("second") != std::string::npos);
    ::unlink(path.c_str());
}

Test(logger_concurrent, parallel_writers_never_interleave_within_a_line, .timeout = 5)
{
    std::ostringstream out;
    Logger logger(out, false);
    constexpr int writers   = 8;
    constexpr int perThread = 200;

    std::vector<std::thread> threads;
    threads.reserve(writers);
    for (int t = 0; t < writers; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < perThread; ++i)
                logger.log("thread=" + std::to_string(t) + " i=" + std::to_string(i));
        });
    }
    for (auto &th : threads)
        th.join();

    std::istringstream in(out.str());
    std::string line;
    int total = 0;
    while (std::getline(in, line)) {
        cr_assert(line.find("[INFO] thread=") != std::string::npos,
            "line was corrupted: '%s'", line.c_str());
        ++total;
    }
    cr_assert_eq(total, writers * perThread);
}

Test(logger_timestamp, timestamp_prefix_is_included_by_default)
{
    std::ostringstream out;
    Logger logger(out);
    logger.log("hi");
    const std::string contents = out.str();
    cr_assert_eq(contents.front(), '[');
}
