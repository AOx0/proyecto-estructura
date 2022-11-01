#include <gtest/gtest.h>

#include "../src/lib/serializer.hpp"
#include "../src/lib/table.hpp"

TEST(BinSerde, StructToBytes) {
  Layout input{.size = 234233, .optional=false, .type=ColumnType::f64};  
  DynArray result = serialize_layout(input);
  Layout cmp = deserialize_layout(result.array, result.length);
  
  drop_dyn_array(result);
  
  EXPECT_EQ(input, cmp);
}
