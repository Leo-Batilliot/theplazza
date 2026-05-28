#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "ipc/Serializer.hpp"
#include "pizza/Types.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using plazza::IpcError;
using plazza::ipc::Deserializer;
using plazza::ipc::Serializer;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;

Test(serializer_primitives, uint8_round_trip_min_mid_max)
{
    Serializer s;
    s.writeUInt8(0);
    s.writeUInt8(123);
    s.writeUInt8(std::numeric_limits<std::uint8_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readUInt8(), 0u);
    cr_assert_eq(d.readUInt8(), 123u);
    cr_assert_eq(d.readUInt8(), std::numeric_limits<std::uint8_t>::max());
    cr_assert(d.eof());
}

Test(serializer_primitives, uint16_round_trip)
{
    Serializer s;
    s.writeUInt16(0);
    s.writeUInt16(0x1234);
    s.writeUInt16(std::numeric_limits<std::uint16_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readUInt16(), 0u);
    cr_assert_eq(d.readUInt16(), 0x1234u);
    cr_assert_eq(d.readUInt16(), std::numeric_limits<std::uint16_t>::max());
}

Test(serializer_primitives, uint32_round_trip)
{
    Serializer s;
    s.writeUInt32(0);
    s.writeUInt32(0xDEADBEEFu);
    s.writeUInt32(std::numeric_limits<std::uint32_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readUInt32(), 0u);
    cr_assert_eq(d.readUInt32(), 0xDEADBEEFu);
    cr_assert_eq(d.readUInt32(), std::numeric_limits<std::uint32_t>::max());
}

Test(serializer_primitives, uint64_round_trip)
{
    Serializer s;
    s.writeUInt64(0);
    s.writeUInt64(0x0123456789ABCDEFull);
    s.writeUInt64(std::numeric_limits<std::uint64_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readUInt64(), 0ull);
    cr_assert_eq(d.readUInt64(), 0x0123456789ABCDEFull);
    cr_assert_eq(d.readUInt64(), std::numeric_limits<std::uint64_t>::max());
}

Test(serializer_primitives, int32_round_trip_negative_values_preserved)
{
    Serializer s;
    s.writeInt32(-1);
    s.writeInt32(std::numeric_limits<std::int32_t>::min());
    s.writeInt32(std::numeric_limits<std::int32_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readInt32(), -1);
    cr_assert_eq(d.readInt32(), std::numeric_limits<std::int32_t>::min());
    cr_assert_eq(d.readInt32(), std::numeric_limits<std::int32_t>::max());
}

Test(serializer_primitives, int64_round_trip_negative_values_preserved)
{
    Serializer s;
    s.writeInt64(-42);
    s.writeInt64(std::numeric_limits<std::int64_t>::min());
    s.writeInt64(std::numeric_limits<std::int64_t>::max());

    Deserializer d(s.take());
    cr_assert_eq(d.readInt64(), -42);
    cr_assert_eq(d.readInt64(), std::numeric_limits<std::int64_t>::min());
    cr_assert_eq(d.readInt64(), std::numeric_limits<std::int64_t>::max());
}

Test(serializer_primitives, bool_round_trip)
{
    Serializer s;
    s.writeBool(true);
    s.writeBool(false);

    Deserializer d(s.take());
    cr_assert_eq(d.readBool(), true);
    cr_assert_eq(d.readBool(), false);
}

Test(serializer_endianness, uint32_wire_format_is_little_endian)
{
    Serializer s;
    s.writeUInt32(0xDEADBEEFu);

    const auto &bytes = s.data();
    cr_assert_eq(bytes.size(), 4u);
    cr_assert_eq(bytes[0], 0xEFu);
    cr_assert_eq(bytes[1], 0xBEu);
    cr_assert_eq(bytes[2], 0xADu);
    cr_assert_eq(bytes[3], 0xDEu);
}

Test(serializer_endianness, uint16_wire_format_is_little_endian)
{
    Serializer s;
    s.writeUInt16(0x1234);

    const auto &bytes = s.data();
    cr_assert_eq(bytes.size(), 2u);
    cr_assert_eq(bytes[0], 0x34u);
    cr_assert_eq(bytes[1], 0x12u);
}

Test(serializer_string, empty_string_round_trip)
{
    Serializer s;
    s.writeString("");

    Deserializer d(s.take());
    const auto out = d.readString();
    cr_assert_eq(out.size(), 0u);
    cr_assert(d.eof());
}

Test(serializer_string, ascii_string_round_trip)
{
    Serializer s;
    s.writeString("regina");

    Deserializer d(s.take());
    cr_assert_str_eq(d.readString().c_str(), "regina");
}

Test(serializer_string, embedded_null_byte_preserved)
{
    const std::string payload("abc\0def", 7);
    Serializer s;
    s.writeString(payload);

    Deserializer d(s.take());
    const auto out = d.readString();
    cr_assert_eq(out.size(), 7u);
    cr_assert_eq(out, payload);
}

Test(serializer_string, utf8_string_round_trip)
{
    const std::string payload = "fantasia\xC3\xA9\xC3\xA8";
    Serializer s;
    s.writeString(payload);

    Deserializer d(s.take());
    cr_assert_eq(d.readString(), payload);
}

Test(serializer_string, length_prefix_is_uint32_little_endian)
{
    Serializer s;
    s.writeString("ab");

    const auto &bytes = s.data();
    cr_assert_eq(bytes.size(), 6u);
    cr_assert_eq(bytes[0], 0x02u);
    cr_assert_eq(bytes[1], 0x00u);
    cr_assert_eq(bytes[2], 0x00u);
    cr_assert_eq(bytes[3], 0x00u);
    cr_assert_eq(bytes[4], static_cast<std::uint8_t>('a'));
    cr_assert_eq(bytes[5], static_cast<std::uint8_t>('b'));
}

Test(serializer_enum, pizza_type_round_trip_preserves_underlying_value)
{
    Serializer s;
    s.writeEnum(PizzaType::Regina);
    s.writeEnum(PizzaType::Margarita);
    s.writeEnum(PizzaType::Americana);
    s.writeEnum(PizzaType::Fantasia);

    Deserializer d(s.take());
    cr_assert_eq(d.readEnum<PizzaType>(), PizzaType::Regina);
    cr_assert_eq(d.readEnum<PizzaType>(), PizzaType::Margarita);
    cr_assert_eq(d.readEnum<PizzaType>(), PizzaType::Americana);
    cr_assert_eq(d.readEnum<PizzaType>(), PizzaType::Fantasia);
}

Test(serializer_enum, pizza_size_round_trip_preserves_underlying_value)
{
    Serializer s;
    s.writeEnum(PizzaSize::S);
    s.writeEnum(PizzaSize::M);
    s.writeEnum(PizzaSize::L);
    s.writeEnum(PizzaSize::XL);
    s.writeEnum(PizzaSize::XXL);

    Deserializer d(s.take());
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::S);
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::M);
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::L);
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::XL);
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::XXL);
}

Test(serializer_mixed, interleaved_fields_round_trip)
{
    Serializer s;
    s.writeEnum(PizzaType::Regina);
    s.writeEnum(PizzaSize::XXL);
    s.writeInt32(-7);
    s.writeString("client-42");
    s.writeBool(true);
    s.writeUInt64(0xFEEDFACECAFEBEEFull);

    Deserializer d(s.take());
    cr_assert_eq(d.readEnum<PizzaType>(), PizzaType::Regina);
    cr_assert_eq(d.readEnum<PizzaSize>(), PizzaSize::XXL);
    cr_assert_eq(d.readInt32(), -7);
    cr_assert_str_eq(d.readString().c_str(), "client-42");
    cr_assert_eq(d.readBool(), true);
    cr_assert_eq(d.readUInt64(), 0xFEEDFACECAFEBEEFull);
    cr_assert(d.eof());
}

Test(serializer_state, take_resets_buffer_to_empty)
{
    Serializer s;
    s.writeUInt32(42);
    cr_assert_eq(s.size(), 4u);

    const auto taken = s.take();
    cr_assert_eq(taken.size(), 4u);
    cr_assert_eq(s.size(), 0u);
}

Test(serializer_state, clear_resets_buffer_to_empty)
{
    Serializer s;
    s.writeUInt32(42);
    s.writeString("foo");
    s.clear();
    cr_assert_eq(s.size(), 0u);
}

Test(serializer_state, remaining_decreases_after_each_read)
{
    Serializer s;
    s.writeUInt32(1);
    s.writeUInt32(2);

    Deserializer d(s.take());
    cr_assert_eq(d.remaining(), 8u);
    (void) d.readUInt32();
    cr_assert_eq(d.remaining(), 4u);
    (void) d.readUInt32();
    cr_assert_eq(d.remaining(), 0u);
    cr_assert(d.eof());
}

Test(deserializer_errors, underrun_on_uint32_throws)
{
    const std::vector<std::uint8_t> partial{0x01, 0x02, 0x03};
    Deserializer d(partial);

    cr_assert_throw(d.readUInt32(), IpcError);
}

Test(deserializer_errors, underrun_on_string_payload_throws)
{
    Serializer s;
    s.writeUInt32(10);
    s.writeUInt8('a');
    s.writeUInt8('b');

    Deserializer d(s.take());
    cr_assert_throw(d.readString(), IpcError);
}

Test(deserializer_errors, invalid_bool_encoding_throws)
{
    const std::vector<std::uint8_t> bad{0x07};
    Deserializer d(bad);

    cr_assert_throw(d.readBool(), IpcError);
}

Test(deserializer_errors, empty_buffer_eof_immediately)
{
    Deserializer d(std::vector<std::uint8_t>{});

    cr_assert(d.eof());
    cr_assert_eq(d.remaining(), 0u);
    cr_assert_throw(d.readUInt8(), IpcError);
}

Test(deserializer_construction, from_raw_pointer_and_size)
{
    const std::uint8_t bytes[] = {0xEF, 0xBE, 0xAD, 0xDE};
    Deserializer d(bytes, sizeof(bytes));

    cr_assert_eq(d.readUInt32(), 0xDEADBEEFu);
    cr_assert(d.eof());
}
