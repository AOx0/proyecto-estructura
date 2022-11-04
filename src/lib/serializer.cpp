#include "serializer.hpp"

Arr sLayout(Layout layout) {
  return {__rsp::serialize_layout(layout)};
}

Layout dLayout(uint8_t * a, uint64_t aa) {
  return {__rsp::deserialize_layout(a, aa)};
}
