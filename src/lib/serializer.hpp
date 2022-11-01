#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include "table.hpp"


struct DynArray {
  uint8_t * array;
  uint64_t length;  
};

extern "C" void drop_dyn_array(DynArray); 

extern "C" DynArray serialize_layout(Layout);
extern "C" Layout deserialize_layout(uint8_t *, uint64_t);

#endif //SERIALIZER_HPP