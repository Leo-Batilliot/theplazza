#pragma once

#include "ipc/Messages.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace plazza::reception {

class StatusCommand {
public:
    static std::string formatReply(const ipc::StatusReplyMsg &reply);
    static std::string formatHandle(std::int32_t kitchenId, std::size_t inFlight,
        std::size_t maxConcurrent);
    static std::string formatSummary(std::size_t kitchenCount,
        const std::vector<ipc::StatusReplyMsg> &replies);
    static std::string ingredientName(std::size_t index);
};

}
