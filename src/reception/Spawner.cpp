#include "reception/Spawner.hpp"

#include "common/Errors.hpp"
#include "ipc/NamedPipeChannel.hpp"

#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>

namespace plazza::reception {

namespace {

void makePipeOrThrow(int fds[2])
{
    if (::pipe(fds) != 0)
        throw plazza::IpcError(std::string("Spawner: pipe failed: ") + std::strerror(errno), errno);
}

}

Spawner::Spawner(kitchen::KitchenConfig defaults) : _defaults(defaults) {}

SpawnedKitchen Spawner::spawn(std::int32_t id)
{
    int r2k[2];
    int k2r[2];
    makePipeOrThrow(r2k);
    makePipeOrThrow(k2r);

    kitchen::KitchenConfig cfg = _defaults;
    cfg.id = id;
    const int childRead  = r2k[0];
    const int childWrite = k2r[1];
    const int parentRead  = k2r[0];
    const int parentWrite = r2k[1];

    concurrency::Process process([childRead, childWrite, parentRead, parentWrite, cfg]() -> int {
        ::close(parentRead);
        ::close(parentWrite);
        std::signal(SIGPIPE, SIG_IGN);
        try {
            ipc::NamedPipeChannel channel(childRead, childWrite);
            kitchen::Kitchen kitchen(cfg);
            kitchen.run(channel);
        } catch (...) {
            return 1;
        }
        return 0;
    });

    ::close(childRead);
    ::close(childWrite);

    auto channel = std::make_unique<ipc::NamedPipeChannel>(parentRead, parentWrite);
    auto handle  = std::make_unique<KitchenHandle>(id, std::move(channel), cfg.maxConcurrentPizzas());

    SpawnedKitchen result;
    result.handle  = std::move(handle);
    result.process = std::move(process);
    return result;
}

}
