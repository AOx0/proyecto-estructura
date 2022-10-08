#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <iostream>
#include <string>
#include <vector>

namespace Tokenizer {
  std::vector<std::string> tokenize(const std::string &in);

  bool validate(const std::string &in);
};

#endif //TOKENIZER_HPP
