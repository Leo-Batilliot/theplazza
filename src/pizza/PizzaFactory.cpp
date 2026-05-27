#include "pizza/PizzaFactory.hpp"

#include "common/Errors.hpp"

namespace plazza::pizza {

PizzaFactory &PizzaFactory::instance()
{
    static PizzaFactory factory;
    return factory;
}

void PizzaFactory::registerPizza(std::string name, PizzaType type, Creator creator)
{
    if (name.empty())
        throw Error("PizzaFactory::registerPizza: empty pizza name");
    if (!creator)
        throw Error("PizzaFactory::registerPizza: null creator for '" + name + "'");
    if (_byType.find(type) != _byType.end())
        throw Error("PizzaFactory::registerPizza: type already registered for '" + name + "'");
    if (_byName.find(name) != _byName.end())
        throw Error("PizzaFactory::registerPizza: name '" + name + "' already registered");

    _byName.emplace(name, type);
    _byType.emplace(type, Entry{std::move(name), std::move(creator)});
}

std::unique_ptr<Pizza> PizzaFactory::create(PizzaType type, PizzaSize size) const
{
    const auto it = _byType.find(type);
    if (it == _byType.end())
        throw Error("PizzaFactory::create: unregistered pizza type");
    return it->second.creator(size);
}

std::unique_ptr<Pizza> PizzaFactory::createByName(const std::string &name, PizzaSize size) const
{
    const auto it = _byName.find(name);
    if (it == _byName.end())
        throw Error("PizzaFactory::createByName: unknown pizza '" + name + "'");
    return create(it->second, size);
}

bool PizzaFactory::isRegistered(PizzaType type) const
{
    return _byType.find(type) != _byType.end();
}

bool PizzaFactory::isRegisteredName(const std::string &name) const
{
    return _byName.find(name) != _byName.end();
}

std::optional<PizzaType> PizzaFactory::typeForName(const std::string &name) const
{
    const auto it = _byName.find(name);
    if (it == _byName.end())
        return std::nullopt;
    return it->second;
}

std::optional<std::string> PizzaFactory::nameForType(PizzaType type) const
{
    const auto it = _byType.find(type);
    if (it == _byType.end())
        return std::nullopt;
    return it->second.name;
}

std::vector<std::string> PizzaFactory::registeredNames() const
{
    std::vector<std::string> names;
    names.reserve(_byType.size());
    for (const auto &[_, entry] : _byType)
        names.push_back(entry.name);
    return names;
}

void PizzaFactory::clear()
{
    _byType.clear();
    _byName.clear();
}

}
