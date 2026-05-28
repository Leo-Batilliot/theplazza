#include "ipc/Serializer.hpp"

#include "common/Errors.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace plazza::ipc {

void Serializer::writeUInt8(std::uint8_t value)
{
    _buffer.push_back(value);
}

void Serializer::writeUInt16(std::uint16_t value)
{
    _buffer.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    _buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

void Serializer::writeUInt32(std::uint32_t value)
{
    _buffer.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    _buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    _buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    _buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

void Serializer::writeUInt64(std::uint64_t value)
{
    for (unsigned shift = 0; shift < 64; shift += 8)
        _buffer.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
}

void Serializer::writeInt32(std::int32_t value)
{
    writeUInt32(static_cast<std::uint32_t>(value));
}

void Serializer::writeInt64(std::int64_t value)
{
    writeUInt64(static_cast<std::uint64_t>(value));
}

void Serializer::writeBool(bool value)
{
    _buffer.push_back(value ? std::uint8_t{1} : std::uint8_t{0});
}

void Serializer::writeString(const std::string &value)
{
    if (value.size() > std::numeric_limits<std::uint32_t>::max())
        throw IpcError("Serializer::writeString: string exceeds 4 GiB limit", 0);
    const auto length = static_cast<std::uint32_t>(value.size());
    writeUInt32(length);
    _buffer.insert(_buffer.end(), value.begin(), value.end());
}

Deserializer::Deserializer(std::vector<std::uint8_t> buffer)
    : _buffer(std::move(buffer)), _position(0)
{}

Deserializer::Deserializer(const std::uint8_t *data, std::size_t size)
    : _buffer(data, data + size), _position(0)
{}

void Deserializer::ensureAvailable(std::size_t count) const
{
    if (_position + count > _buffer.size())
        throw IpcError("Deserializer: unexpected end of buffer (need " +
            std::to_string(count) + " bytes, have " +
            std::to_string(_buffer.size() - _position) + ")", 0);
}

std::uint8_t Deserializer::readUInt8()
{
    ensureAvailable(1);
    return _buffer[_position++];
}

std::uint16_t Deserializer::readUInt16()
{
    ensureAvailable(2);
    const auto lo = static_cast<std::uint16_t>(_buffer[_position]);
    const auto hi = static_cast<std::uint16_t>(_buffer[_position + 1]);
    _position += 2;
    return static_cast<std::uint16_t>(lo | (hi << 8));
}

std::uint32_t Deserializer::readUInt32()
{
    ensureAvailable(4);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(_buffer[_position + i]) << (i * 8);
    _position += 4;
    return value;
}

std::uint64_t Deserializer::readUInt64()
{
    ensureAvailable(8);
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(_buffer[_position + i]) << (i * 8);
    _position += 8;
    return value;
}

std::int32_t Deserializer::readInt32()
{
    return static_cast<std::int32_t>(readUInt32());
}

std::int64_t Deserializer::readInt64()
{
    return static_cast<std::int64_t>(readUInt64());
}

bool Deserializer::readBool()
{
    const std::uint8_t raw = readUInt8();
    if (raw > 1)
        throw IpcError("Deserializer::readBool: invalid bool encoding " +
            std::to_string(raw), 0);
    return raw != 0;
}

std::string Deserializer::readString()
{
    const std::uint32_t length = readUInt32();
    ensureAvailable(length);
    std::string out(reinterpret_cast<const char *>(_buffer.data() + _position), length);
    _position += length;
    return out;
}

}
