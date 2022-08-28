#ifndef LIB_OPTION_HPP
#define LIB_OPTION_HPP

#include <iostream>
#include "result.hpp"

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

#endif // !LIB_OPTION_HPP
