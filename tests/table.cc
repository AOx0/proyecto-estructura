#include <gtest/gtest.h>

#include "../src/lib/table.hpp"


TEST(Serializer, StructToBytes) {
  const int SIZE = 8;
  const Type TYPE = Type::i8;
  const bool OPTIONAL = true;

  const Table input { .rows = { { "hello", { .size = SIZE, .optional = OPTIONAL, .type = TYPE } } } };

  std::vector<uint8_t> expected { 'h', 'e', 'l', 'l', 'o', '\0', TYPE, SIZE, OPTIONAL, '\0' };
  std::vector<uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result) << "Bad Table struct serialization";
}


TEST(Serializer, MultipleEntry_StructToBytes) {
  const uint8_t SIZE = 8;
  const Type TYPE = Type::i8;
  const bool OPTIONAL = true;

  const Table input { .rows = {
      { "hello", { .size = SIZE, .optional = OPTIONAL, .type = TYPE } },
      { "hello2", { .size = SIZE, .optional = OPTIONAL, .type = TYPE } }
  } };

  std::vector<uint8_t> expected {
      'h', 'e', 'l', 'l', 'o', '\0', TYPE, SIZE, OPTIONAL,
      'h', 'e', 'l', 'l', 'o', '2', '\0', TYPE, SIZE, OPTIONAL,
      '\0'
  };
  std::vector<uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result) << "Bad Table struct serialization";
}

TEST(DeSerializer, BytesToStruct) {
  const uint8_t SIZE = 16;
  const Type TYPE = Type::u16;
  const bool OPTIONAL = false;

  std::vector<uint8_t> input {
    'h', 'e', 'l', 'l', 'o', '\0', TYPE, SIZE, OPTIONAL,
    'h', 'e', 'l', 'l', 'o', '2', '\0', TYPE, SIZE, OPTIONAL,
    '\0'
  };

  const Table expected { .rows = {
      { "hello", { .size = SIZE, .optional = OPTIONAL, .type = TYPE } },
      { "hello2", { .size = SIZE, .optional = OPTIONAL, .type = TYPE } }
  } };
  const Table result(Table::from_vec(input));

  EXPECT_EQ(expected, result) << "Bad Table struct de-serialization";
}
