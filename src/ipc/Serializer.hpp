#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace plazza::ipc {

class Serializer {
public:
    Serializer() = default;
    ~Serializer() = default;

    Serializer(const Serializer &) = default;
    Serializer &operator=(const Serializer &) = default;
    Serializer(Serializer &&) noexcept = default;
    Serializer &operator=(Serializer &&) noexcept = default;

    void writeUInt8(std::uint8_t value);
    void writeUInt16(std::uint16_t value);
    void writeUInt32(std::uint32_t value);
    void writeUInt64(std::uint64_t value);
    void writeInt32(std::int32_t value);
    void writeInt64(std::int64_t value);
    void writeBool(bool value);
    void writeString(const std::string &value);

    template <typename Enum>
    void writeEnum(Enum value)
    {
        static_assert(std::is_enum_v<Enum>, "writeEnum requires an enum type");
        writeUInt32(static_cast<std::uint32_t>(value));
    }

    [[nodiscard]] const std::vector<std::uint8_t> &data() const noexcept { return _buffer; }
    [[nodiscard]] std::size_t size() const noexcept { return _buffer.size(); }
    [[nodiscard]] std::vector<std::uint8_t> take() noexcept { return std::move(_buffer); }
    void clear() noexcept { _buffer.clear(); }

private:
    std::vector<std::uint8_t> _buffer;
};

class Deserializer {
public:
    explicit Deserializer(std::vector<std::uint8_t> buffer);
    Deserializer(const std::uint8_t *data, std::size_t size);
    ~Deserializer() = default;

    Deserializer(const Deserializer &) = default;
    Deserializer &operator=(const Deserializer &) = default;
    Deserializer(Deserializer &&) noexcept = default;
    Deserializer &operator=(Deserializer &&) noexcept = default;

    std::uint8_t readUInt8();
    std::uint16_t readUInt16();
    std::uint32_t readUInt32();
    std::uint64_t readUInt64();
    std::int32_t readInt32();
    std::int64_t readInt64();
    bool readBool();
    std::string readString();

    template <typename Enum>
    Enum readEnum()
    {
        static_assert(std::is_enum_v<Enum>, "readEnum requires an enum type");
        return static_cast<Enum>(readUInt32());
    }

    [[nodiscard]] bool eof() const noexcept { return _position >= _buffer.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return _buffer.size() - _position; }
    [[nodiscard]] std::size_t position() const noexcept { return _position; }

private:
    void ensureAvailable(std::size_t count) const;

    std::vector<std::uint8_t> _buffer;
    std::size_t _position;
};

}
