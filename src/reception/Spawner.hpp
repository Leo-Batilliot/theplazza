#pragma once

#include "concurrency/Process.hpp"
#include "kitchen/Kitchen.hpp"
#include "reception/KitchenHandle.hpp"

#include <cstdint>
#include <memory>

namespace plazza::reception {

struct SpawnedKitchen {
    std::unique_ptr<KitchenHandle> handle;
    concurrency::Process process;
};

class Spawner {
public:
    explicit Spawner(kitchen::KitchenConfig defaults);

    Spawner(const Spawner &) = delete;
    Spawner &operator=(const Spawner &) = delete;

    SpawnedKitchen spawn(std::int32_t id);

    [[nodiscard]] const kitchen::KitchenConfig &defaults() const noexcept { return _defaults; }

private:
    kitchen::KitchenConfig _defaults;
};

}
