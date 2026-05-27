#pragma once

#include <sys/types.h>

#include <functional>
#include <optional>

namespace plazza::concurrency {

struct ExitStatus {
    bool exited = false;
    int code = 0;
    bool signaled = false;
    int signal = 0;
};

class Process {
public:
    using Entry = std::function<int()>;

    Process() noexcept = default;
    explicit Process(Entry entry);
    ~Process();

    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;

    Process(Process &&other) noexcept;
    Process &operator=(Process &&other) noexcept;

    [[nodiscard]] bool joinable() const noexcept { return _pid > 0; }
    [[nodiscard]] pid_t pid() const noexcept { return _pid; }

    ExitStatus wait();
    std::optional<ExitStatus> try_wait();
    void kill(int signal);
    void detach() noexcept;

private:
    pid_t _pid = 0;
};

}
