#include <gtest/gtest.h>

#include "../src/lib/fm.hpp"
#include "../src/lib/table.hpp"

TEST(SerDe, StructToBytes) {
    const int SIZE = 8;
    const Type TYPE = Type::i8;
    const bool OPTIONAL = true;

    const Table input{
        .rows = {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}}};

    std::vector<uint8_t> expected{'h',  'e',  'l',  'l',      'o',
                                  '\0', TYPE, SIZE, OPTIONAL, '\0'};
    std::vector<uint8_t> result(input.into_vec());

    EXPECT_EQ(expected, result) << "Bad Table struct serialization";
}

TEST(SerDe, MultipleEntry_StructToBytes) {
    const uint8_t SIZE = 8;
    const Type TYPE = Type::i8;
    const bool OPTIONAL = true;

    const Table input{
        .rows = {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
            {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}
        }};

    std::vector<uint8_t> expected{
        'h', 'e', 'l', 'l', 'o', '\0', TYPE, SIZE, OPTIONAL, 'h',
        'e', 'l', 'l', 'o', '2', '\0', TYPE, SIZE, OPTIONAL, '\0'};
    std::vector<uint8_t> result(input.into_vec());

    EXPECT_EQ(expected, result) << "Bad Table struct serialization";
}

TEST(SerDe, BytesToStruct) {
    const uint8_t SIZE = 16;
    const Type TYPE = Type::u16;
    const bool OPTIONAL = false;

    std::vector<uint8_t> input{'h',  'e',      'l',  'l',  'o',      '\0', TYPE,
                               SIZE, OPTIONAL, 'h',  'e',  'l',      'l',  'o',
                               '2',  '\0',     TYPE, SIZE, OPTIONAL, '\0'};

    const Table expected{
        .rows = {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
            {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}
        }};
    const Table result(Table::from_vec(input));

    EXPECT_EQ(expected, result) << "Bad Table struct de-serialization";
}

TEST(SerDe, StructToBytesAndViceversa) {
    const int SIZE = 8;
    const Type TYPE = Type::i8;
    const bool OPTIONAL = true;

    const Table input{
        .rows = {{"@$pq=", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}}};

    std::vector<uint8_t> expected{'@',  '$',  'p',  'q',      '=',
                                  '\0', TYPE, SIZE, OPTIONAL, '\0'};
    std::vector<uint8_t> result(input.into_vec());

    EXPECT_EQ(expected, result) << "Bad Table struct serialization";

    const Table result_2(Table::from_vec(result));

    EXPECT_EQ(input, result_2) << "Bad Table struct de-serialization";
}

TEST(SerDe, SaveLoadFromFile) {
    const int SIZE = 8;
    const Type TYPE = Type::i8;
    const bool OPTIONAL = true;

    const Table expected{
        .rows = {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
            {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}
        }};

    expected.to_file("./table1");

    Table result = Table::from_file("./table1");

    EXPECT_EQ(expected, result) << "Bad save & load";
}
