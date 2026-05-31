#include "reception/StatusCommand.hpp"

#include "pizza/Types.hpp"

#include <array>
#include <sstream>

namespace plazza::reception {

namespace {

constexpr std::array<const char *, pizza::ingredientCount> ingredientNames{
    "dough", "tomato", "gruyere", "ham", "mushrooms", "steak", "eggplant", "goat", "chief",
};

}

std::string StatusCommand::ingredientName(std::size_t index)
{
    if (index >= ingredientNames.size())
        return "?";
    return ingredientNames[index];
}

std::string StatusCommand::formatHandle(std::int32_t kitchenId, std::size_t inFlight,
    std::size_t maxConcurrent)
{
    std::ostringstream oss;
    oss << "k" << kitchenId << " inFlight=" << inFlight << "/" << maxConcurrent;
    return oss.str();
}

std::string StatusCommand::formatReply(const ipc::StatusReplyMsg &reply)
{
    std::ostringstream oss;
    oss << "k" << reply.kitchenId
        << " cooks=" << reply.cooksBusy << "/" << reply.cooksTotal
        << " queue=" << reply.queueSize
        << " stock={";
    for (std::size_t i = 0; i < reply.stock.size(); ++i) {
        if (i > 0)
            oss << ",";
        oss << ingredientName(i) << ":" << reply.stock[i];
    }
    oss << "}";
    return oss.str();
}

std::string StatusCommand::formatSummary(std::size_t kitchenCount,
    const std::vector<ipc::StatusReplyMsg> &replies)
{
    std::ostringstream oss;
    oss << "status: " << kitchenCount << " kitchens";
    for (const auto &reply : replies)
        oss << " | " << formatReply(reply);
    return oss.str();
}

}
