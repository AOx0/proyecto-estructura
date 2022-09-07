#ifndef LIB_RESULT_HPP
#define LIB_RESULT_HPP

#include <iostream>

enum ResultType {
  Ok,
  Err
};


template<typename T, typename E>
class Result {
protected:
  T item;
  E error;
  bool dev_asked_first;
  static const char * err_name;
  static const char * valid_name;
  bool valid;
  ResultType type;

  Result (E error, ResultType type)
    : type(type)
    , error(std::move(error))
    , valid(true)
    , dev_asked_first(false)
  { }

  Result (ResultType type, T value)
    : type(type)
    , item(std::move(value))
    , valid(true)
    , dev_asked_first(false)
  { }

  void inline verify(bool for_error = false) {
    if (!dev_asked_first) {
      std::cout << std::endl << "    ERROR: Read without verifying on " << (is_err() ? err_name : valid_name) << std::endl;
      exit(1);
    } else if (is_err() && !for_error) {
      std::cout << std::endl << "    ERROR: Crash when unwraping " << err_name << std::endl;
      exit(1);
    } else if (!valid) {
      std::cout << std::endl << "    ERROR: Crash when unwraping no-longer valir " << valid_name << std::endl;
      exit(1);
    } else if (is_ok() && for_error) {
      std::cout << std::endl << "    ERROR: Crash when unwraping " << valid_name << std::endl;
      exit(1);
    }
  }

public:
  static Result<T, E> Ok(T value) {
    return Result<T, E>(ResultType::Ok, value);
  }

  static Result<T, E> Err(E value) {
    return Result<T, E>(value, ResultType::Err);
  }

  bool is_ok() {
    dev_asked_first = true;
    return this->type == ResultType::Ok;
  }

  bool is_err() {
    return !is_ok();
  }

  inline T unwrap() {
    verify();
    valid = false;
    T i = std::move(this->item);
    return i;
  }

  inline E unwrap_err() {
    verify(true);
    valid = false;
    E i = std::move(this->error);
    return i;
  }


  inline T inner() {
    verify();
    T i = item;
    return i;
  }

  inline E inner_err() {
    verify(true);
    E i = error;
    return i;
  }

};

template<typename T, typename E>
const char * Result<T, E>::err_name = "Result::Err";

template<typename T, typename E>
const char * Result<T, E>::valid_name = "Result::Ok";

#endif // !LIB_RESULT_HPP
