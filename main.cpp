#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <cassert>

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

    Result (ResultType type, T value, E error)
        : type(type)
        , item(std::move(value))
        , error(std::move(error))
        , valid(true)
        , dev_asked_first(false)
    { }

    void inline verify() {
        if (!dev_asked_first) {
            std::cout << std::endl << "    ERROR: Read without verifying on " << (is_err() ? err_name : valid_name) << std::endl;
            exit(1);
        } else if (is_err()) {
            std::cout << std::endl << "    ERROR: Crash when unwraping " << err_name << std::endl;
            exit(1);
        } else if (!valid) {
            std::cout << std::endl << "    ERROR: Crash when unwraping no-longer valir " << valid_name << std::endl;
            exit(1);
        }
    }

public:
    static Result<T, E> Ok(T value) {
        return Result<T, E>(ResultType::Ok, value, (E)NULL);
    }

    static Result<T, E> Err(E value) {
        return Result<T, E>(ResultType::Err, (T)NULL, value);
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

    inline T inner() {
        verify();
        T i = item;
        return i;
    }
};

template<typename T, typename E>
const char * Result<T, E>::err_name = "Result::Err";

template<typename T, typename E>
const char * Result<T, E>::valid_name = "Result::Ok";

template<typename T>
class Option : private Result<T, void *> {
protected:
    using Result<T, void *>::is_ok;
    static const char * err_name;
    static const char * valid_name;

    Option (ResultType type, T value) :  Result<T, void *>(type, value, NULL) { }

public:
    using Result<T, void *>::unwrap;
    using Result<T, void *>::inner;

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
};

template<typename T>
const char * Option<T>::err_name = "Option::None";

template<typename T>
const char * Option<T>::valid_name = "Option::Some";

int main() {
    Option<int> v = Option<int>::Some(4);

    //std::cout << "El valor es " << v.inner() << std::endl;

    if (v.is_some()) {
        std::cout << "El valor es " << v.inner() << std::endl;
    }

    std::cout << "Hello World!" << std::endl;
    return 0;
}

