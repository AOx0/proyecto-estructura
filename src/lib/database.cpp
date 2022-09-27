#include "database.hpp"
#include <vector>

std::vector<std::uint8_t> DataBase::into_vec() const {
    std::vector<std::uint8_t> resultado{};

    for (auto &table : tables) {
        const char *str = table.c_str();
        size_t i = 0;
        while (str[i] != '\0') {
            resultado.push_back(str[i]);
            i++;
        }
        resultado.push_back('\0');
    }

    resultado.push_back('\0');
    return resultado;
}
