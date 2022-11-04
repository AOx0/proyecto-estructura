#include <gtest/gtest.h>

#include "../src/lib/serializer.hpp"
#include "../src/lib/table.hpp"

TEST(BinSerde, StructToBytes) {
  Layout input{.size = 234233, .optional=false, .type=ColumnType::f64};  
  Arr result = sLayout(input);
  Layout cmp = dLayout(*result, result.len());
    
  EXPECT_EQ(input, cmp);
}
