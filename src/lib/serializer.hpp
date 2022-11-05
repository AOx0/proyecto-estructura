#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <iostream>
#include "analyzer/parser.hpp"

enum ColumnType {
  u8 = 1,
  u16 = 2,
  u32 = 11,
  u64 = 3,
  i8 = 4,
  i16 = 5,
  i32 = 10,
  i64 = 6,
  f32 = 7,
  f64 = 8,
  str = 9,
  rbool = 12
};

std::string to_string(ColumnType type);

struct Layout {
  uint64_t size;
  bool optional;
  ColumnType type;

  bool operator==(Layout const &other) const;

  bool operator!=(Layout const &other) const {
    return !(other == *this);
  }

  static Layout from_parser_type(Parser::Type const &type) {
    switch (type.variant) {
      case Parser::TypeE::U8:
        return Layout{.size = 1, .optional = false, .type = ColumnType::u8};
      case Parser::TypeE::U16:
        return Layout{.size = 2, .optional = false, .type = ColumnType::u16};
      case Parser::TypeE::U32:
        return Layout{.size = 4, .optional = false, .type = ColumnType::u32};
      case Parser::TypeE::U64:
        return Layout{.size = 8, .optional = false, .type = ColumnType::u64};
      case Parser::TypeE::I8:
        return Layout{.size = 1, .optional = false, .type = ColumnType::i8};
      case Parser::TypeE::I16:
        return Layout{.size = 2, .optional = false, .type = ColumnType::i16};
      case Parser::TypeE::I32:
        return Layout{.size = 4, .optional = false, .type = ColumnType::i32};
      case Parser::TypeE::I64:
        return Layout{.size = 8, .optional = false, .type = ColumnType::i64};
        //case Parser::TypeE::F32:
        //  return Layout{.size = 4, .optional = false, .type = ColumnType::f32};
      case Parser::TypeE::F64:
        return Layout{.size = 8, .optional = false, .type = ColumnType::f64};
      case Parser::TypeE::STR:
        return Layout{.size = 0, .optional = false, .type = ColumnType::str};
      case Parser::TypeE::BOOL:
        return Layout{.size = 1, .optional = false, .type = ColumnType::rbool};
      default:
        throw std::runtime_error("Invalid type");
    }
  }

  std::string ly_to_string() {
    std::stringstream result;
    result << *this;
    return result.str();
  }

  friend std::ostream &operator<<(std::ostream &os, Layout const &layout) {
    os << to_string(layout.type) << "(" << layout.size << " bytes)";
    //os << "Layout{size=" << layout.size << ", type=" << layout.type << "}";
    return os;
  }
};

struct DynArray {
  uint8_t * array;
  uint64_t length;  
};

namespace rsp_ {
  extern "C" void drop_dyn_array(DynArray); 
  extern "C" DynArray serialize_layout(Layout);
  extern "C" Layout deserialize_layout(uint8_t *, uint64_t);

  extern "C" DynArray su64(uint64_t);
  extern "C" uint64_t du64(uint8_t *, uint64_t);
  extern "C" DynArray su32(uint32_t);
  extern "C" uint32_t du32(uint8_t *, uint64_t);
  extern "C" DynArray su16(uint16_t);
  extern "C" uint16_t du16(uint8_t *, uint64_t);
  extern "C" DynArray su8(uint8_t);
  extern "C" uint8_t du8(uint8_t *, uint64_t);

  extern "C" DynArray si64(int64_t);
  extern "C" int32_t di64(uint8_t *, uint64_t);
  extern "C" DynArray si32(int32_t);
  extern "C" int32_t di32(uint8_t *, uint64_t);
  extern "C" DynArray si16(int16_t);
  extern "C" int16_t di16(uint8_t *, uint64_t);
  extern "C" DynArray si8(int8_t);
  extern "C" int8_t di8(uint8_t *, uint64_t);

  extern "C" DynArray sf64(double);
  extern "C" double df64(uint8_t *, uint64_t);

  extern "C" DynArray sbool(bool);
  extern "C" bool dbool(uint8_t *, uint64_t);

  extern "C" DynArray serialize_str(const char *, uint64_t);
  extern "C" char * deserialize_str(uint8_t *, uint64_t);
  extern "C" void drop_str(char *);
}

struct SString {
  char *data;
  uint64_t size;

  SString() : data(nullptr), size(0) {}
  SString(char *data, uint64_t size) : data(data), size(size) {}

  ~SString() {
    rsp_::drop_str(data);
  }

  std::string to_string() {
    return std::string(data);
  }
};

struct Serialized {
protected:
  DynArray member;
public:
  template<typename T>
  uint8_t operator[](T idx) {
    return member.array[idx];
  }

  uint8_t * data() {
    return member.array;
  }
  
  explicit Serialized(const DynArray & array) : member(array) {}
  
  [[nodiscard]] uint64_t len() const {
    return member.length;
  }
  
  ~Serialized() {
    rsp_::drop_dyn_array(member);
  }

  [[nodiscard]] static Layout dLayout(uint8_t * a, uint64_t aa) {
    return {rsp_::deserialize_layout(a, aa)};
  }
  
  [[nodiscard]] Layout dLayout() const {
    return {rsp_::deserialize_layout(member.array, member.length)};
  }

  [[nodiscard]] static uint64_t du64(uint8_t * a, uint64_t aa) {
    return rsp_::du64(a, aa);
  }

  [[nodiscard]] uint64_t du64() const {
    return rsp_::du64(member.array, member.length);
  }

  [[nodiscard]] static uint32_t du32(uint8_t * a, uint64_t aa) {
    return rsp_::du32(a, aa);
  }

  [[nodiscard]] uint32_t du32() const {
    return rsp_::du32(member.array, member.length);
  }

  [[nodiscard]] static uint16_t du16(uint8_t * a, uint64_t aa) {
    return rsp_::du16(a, aa);
  }

  [[nodiscard]] uint16_t du16() const {
    return rsp_::du16(member.array, member.length);
  }

  [[nodiscard]] static uint8_t du8(uint8_t * a, uint64_t aa) {
    return rsp_::du8(a, aa);
  }

  [[nodiscard]] uint8_t du8() const {
    return rsp_::du8(member.array, member.length);
  }

  [[nodiscard]] static int64_t di64(uint8_t * a, uint64_t aa) {
    return rsp_::di64(a, aa);
  }

  [[nodiscard]] int64_t di64() const {
    return rsp_::di64(member.array, member.length);
  }

  [[nodiscard]] static int32_t di32(uint8_t * a, uint64_t aa) {
    return rsp_::di32(a, aa);
  }

  [[nodiscard]] int32_t di32() const {
    return rsp_::di32(member.array, member.length);
  }

  [[nodiscard]] static int16_t di16(uint8_t * a, uint64_t aa) {
    return rsp_::di16(a, aa);
  }

  [[nodiscard]] int16_t di16() const {
    return rsp_::di16(member.array, member.length);
  }

  [[nodiscard]] static int8_t di8(uint8_t * a, uint64_t aa) {
    return rsp_::di8(a, aa);
  }

  [[nodiscard]] int8_t di8() const {
    return rsp_::di8(member.array, member.length);
  }

  [[nodiscard]] static double df64(uint8_t * a, uint64_t aa) {
    return rsp_::df64(a, aa);
  }

  [[nodiscard]] double df64() const {
    return rsp_::df64(member.array, member.length);
  }

  [[nodiscard]] static bool dbool(uint8_t * a, uint64_t aa) {
    return rsp_::dbool(a, aa);
  }

  [[nodiscard]] bool dbool() const {
    return rsp_::dbool(member.array, member.length);
  }

  [[nodiscard]] static SString dstr(uint8_t * a, uint64_t aa) {
    return {rsp_::deserialize_str(a, aa), aa};
  }

  [[nodiscard]] SString dstr() const {
    return {rsp_::deserialize_str(member.array, member.length), member.length};
  }

  static Serialized serialize(bool value) {
    return Serialized(rsp_::sbool(value));
  }

  static Serialized serialize(Layout layout) {
    return Serialized(rsp_::serialize_layout(layout));
  }
  
  static Serialized serialize(uint64_t value) {
    return Serialized(rsp_::su64(value));
  }

  static Serialized serialize(uint32_t value) {
    return Serialized(rsp_::su32(value));
  }

  static Serialized serialize(uint16_t value) {
    return Serialized(rsp_::su16(value));
  }

  static Serialized serialize(uint8_t value) {
    return Serialized(rsp_::su8(value));
  }

  static Serialized serialize(int64_t value) {
    return Serialized(rsp_::si64(value));
  }

  static Serialized serialize(int32_t value) {
    return Serialized(rsp_::si32(value));
  }

  static Serialized serialize(int16_t value) {
    return Serialized(rsp_::si16(value));
  }

  static Serialized serialize(int8_t value) {
    return Serialized(rsp_::si8(value));
  }

  static Serialized serialize(double value) {
    return Serialized(rsp_::sf64(value));
  }

  static Serialized serialize(const char * value, uint64_t size) {
    return Serialized(rsp_::serialize_str(value, size));
  }

  static Serialized serialize(const std::string & value, uint64_t size) {
    return Serialized(rsp_::serialize_str(value.c_str(), size));
  }
};

#endif //SERIALIZER_HPP