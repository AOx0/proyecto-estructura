#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Parser {
std::vector<std::string> tokenize(const std::string &in);

std::tuple<bool, std::optional<std::string>> validate(const std::string &in);

void make_tokens_explicit(std::string &str);
}; // namespace Parser

#endif // TOKENIZER_HPP
