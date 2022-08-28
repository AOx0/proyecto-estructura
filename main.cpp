#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

enum ResultType {
  Ok,
  Err
};


template<typename T, typename E>
class Result {
protected:
  T item;
  E error;
  bool valid;
  ResultType type;
  
  Result (ResultType type, T value, E error) 
    : type(type)
    , item(std::move(value))
    , error(std::move(error))
    , valid(true)
  { }

public:
  static Result<T, E> Ok(T value) {
    return Result<T, E>(ResultType::Ok, value, (E)NULL);
  }

  static Result<T, E> Err(E value) {
    return Result<T, E>(ResultType::Err, (T)NULL, value);
  }

  bool is_ok() {
    return this->type == ResultType::Ok;
  }

  bool is_err() {
    return !is_ok();
  }

  virtual T unwrap() {
    if (this->is_err()) {
      std::cout << "Crash when unwraping Result::Err" << std::endl;
      exit(1);
    } else if (!this->valid) {
      std::cout << "Crash when unwraping no-longer valir Result::Ok" << std::endl;
      exit(1);
    } else {
      valid = false;
      T i = std::move(this->item);
      return i;
    }
  }

  virtual T inner() {
    if (this->is_err()) {
      std::cout << "Crash when unwraping Result::Err" << std::endl;
      exit(1);
    } else if (!this->valid) {
      std::cout << "Crash when unwraping no-longer valir Result::Ok" << std::endl;
      exit(1);
    } else {
      T i = item;
      return i;
    }
  }
};

template<typename T>
class Option : private Result<T, void *> {
private:
  using Result<T, void *>::is_ok;
  using Result<T, void *>::inner;

  Option (ResultType type, T value) :  Result<T, void *>(type, value, NULL) { }

public:
   bool is_some() {
     return is_ok(); 
   }

   bool is_none() {
     return !is_ok();
   }

   static Option<T> Some(T value) {
    return Option<T>(ResultType::Ok, value);
   }
   
   static Option<T> None() {
    return Option<T>(ResultType::Err, (T)NULL);
   }

    T unwrap() override {
    if (this->is_err()) {
      std::cout << "Crash when unwraping Option::None" << std::endl;
      exit(1);
    } else if (!this->valid) {
      std::cout << "Crash when unwraping no-longer valir Option::Some" << std::endl;
      exit(1);
    } else {
      this->valid = false;
      T i = std::move(this->item);
      return i;
    }
  }

  T inner() override {
   if (this->is_err()) {
      std::cout << "Crash when unwraping Option::None" << std::endl;
      exit(1);
    } else if (!this->valid) {
      std::cout << "Crash when unwraping no-longer valid Option::Some" << std::endl;
      exit(1);
    } else {
      T i = this->item;
      return i;
    }

   }

};

Result<int, int> try_get() {
  return Result<int, int>::Err(10);
}

Result<std::string, int> try_get_success() {
  std::string value = std::string("Hola");
  return Result<std::string, int>::Ok(value);
}

int main() {
  Result<std::string, int> r = try_get_success();

  Option<int> v = Option<int>::None();

  if (v.is_some()) {
    std::cout << "El valor es " << v.inner() << std::endl;
  }

  std::cout << "Hello World!" << std::endl;
  return 0;
}
