#pragma once

#include "pizza/Pizza.hpp"
#include "pizza/Types.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace plazza::pizza {

class PizzaFactory {
public:
    using Creator = std::function<std::unique_ptr<Pizza>(PizzaSize)>;

    static PizzaFactory &instance();

    PizzaFactory(const PizzaFactory &) = delete;
    PizzaFactory &operator=(const PizzaFactory &) = delete;
    PizzaFactory(PizzaFactory &&) = delete;
    PizzaFactory &operator=(PizzaFactory &&) = delete;

    void registerPizza(std::string name, PizzaType type, Creator creator);

    [[nodiscard]] std::unique_ptr<Pizza> create(PizzaType type, PizzaSize size) const;
    [[nodiscard]] std::unique_ptr<Pizza> createByName(const std::string &name, PizzaSize size) const;

    [[nodiscard]] bool isRegistered(PizzaType type) const;
    [[nodiscard]] bool isRegisteredName(const std::string &name) const;

    [[nodiscard]] std::optional<PizzaType> typeForName(const std::string &name) const;
    [[nodiscard]] std::optional<std::string> nameForType(PizzaType type) const;

    [[nodiscard]] std::vector<std::string> registeredNames() const;
    [[nodiscard]] std::size_t size() const noexcept { return _byType.size(); }

    void clear();

private:
    PizzaFactory() = default;

    struct Entry {
        std::string name;
        Creator creator;
    };

    std::map<PizzaType, Entry> _byType;
    std::unordered_map<std::string, PizzaType> _byName;
};

template <typename ConcretePizza>
class PizzaRegistrar {
public:
    PizzaRegistrar(std::string name, PizzaType type)
    {
        PizzaFactory::instance().registerPizza(std::move(name), type,
            [](PizzaSize size) -> std::unique_ptr<Pizza> {
                return std::make_unique<ConcretePizza>(size);
            });
    }
};

}
