#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace Tokenizer {
  std::vector<std::string> tokenize(const std::string &in);

  std::tuple<bool, std::optional<std::string>> validate(const std::string &in);

  void make_tokens_explicit(std::string &str);
};

#endif //TOKENIZER_HPP
