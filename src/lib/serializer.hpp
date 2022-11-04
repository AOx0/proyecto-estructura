#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include "table.hpp"


struct DynArray {
  uint8_t * array;
  uint64_t length;  
};

namespace __rsp {
  extern "C" void drop_dyn_array(DynArray); 
  extern "C" DynArray serialize_layout(Layout);
  extern "C" Layout deserialize_layout(uint8_t *, uint64_t);
}

struct Arr {
protected:
  DynArray member;
public:
  uint8_t * operator*() {
    return member.array;
  }
  
  Arr(const DynArray & array) : member(array) {}
  
  uint64_t len() {
    return member.length;
  }
  
  ~Arr() {
    __rsp::drop_dyn_array(member);
  }
};

Arr sLayout(Layout layout);
Layout dLayout(uint8_t *, uint64_t);

#endif //SERIALIZER_HPP