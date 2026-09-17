#include "circuit_escape/cells.hpp"

#include <type_traits>

// std::visit llama a la lambda con la alternativa activa del variant. decltype(value)
// es "const T&"; decay_t lo reduce a T para elegir la especialización correcta.
bool isTraversable(const Cell& cell) {
    return std::visit([](const auto& value) {
        return CellTraits<std::decay_t<decltype(value)>>::traversable;
    }, cell);
}

bool isCollectible(const Cell& cell) {
    return std::visit([](const auto& value) {
        return CellTraits<std::decay_t<decltype(value)>>::collectible;
    }, cell);
}
