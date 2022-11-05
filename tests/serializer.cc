#include <gtest/gtest.h>

#include "../src/lib/serializer.hpp"
#include "../src/lib/table.hpp"

TEST(BinSerde, StructToBytes) {
  Layout input{.size = 234233, .optional=false, .type=ColumnType::f64};  
  Serialized result = Serialized::serialize(input);
  Layout cmp = result.dLayout();
    
  EXPECT_EQ(input, cmp);
}

TEST(BinSerde, uint64_t) {
  uint64_t input = 234233;
  Serialized result = Serialized::serialize(input);
  uint64_t cmp = result.du64();

  EXPECT_EQ(input, cmp);
}

TEST(BinSerde, string2string) {
  std::string nombre = "Daniel";
  
  Serialized result = Serialized::serialize(nombre, 150);
  std::string cmp = result.dstr().to_string();
  
  EXPECT_EQ(nombre, cmp);
}

TEST(BinSerde, uint8) {
  uint8_t input = 45;
  Serialized result = Serialized::serialize(45);
  uint8_t cmp = result.du8();
  
  EXPECT_EQ(input, cmp);
}