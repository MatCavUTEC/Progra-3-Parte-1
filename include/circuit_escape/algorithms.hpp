#pragma once
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

#include "circuit_escape/grid.hpp"
#include "circuit_escape/position.hpp"

// Cuenta los elementos de un rango que cumplen la condición.
template<typename InputIt, typename Predicate>
[[nodiscard]] std::size_t countMatching(InputIt first, InputIt last, Predicate predicate) {
    std::size_t result = 0;
    for (; first != last; ++first) {
        if (predicate(*first)) {
            ++result;
        }
    }
    return result;
}

// Devuelve el elemento de menor costo, o nullopt si el rango está vacío.
// Con costos iguales gana el primero, para que la decisión sea reproducible.
template<typename ForwardIt, typename Cost>
[[nodiscard]] std::optional<typename std::iterator_traits<ForwardIt>::value_type>
minimumBy(ForwardIt first, ForwardIt last, Cost cost) {
    if (first == last) {
        return std::nullopt;
    }
    ForwardIt best = first;
    auto bestCost = cost(*first);
    for (++first; first != last; ++first) {
        auto candidateCost = cost(*first);
        if (candidateCost < bestCost) {
            best = first;
            bestCost = std::move(candidateCost);
        }
    }
    return *best;
}

// Busca la primera celda que cumple la condición, recorriendo por filas, y
// devuelve su posición en el tablero.
template<typename CellType, std::size_t Rows, std::size_t Columns, typename Predicate>
[[nodiscard]] std::optional<Position> findPosition(const Grid<CellType, Rows, Columns>& grid,
                                                   Predicate predicate) {
    std::size_t index = 0;
    for (auto cell = grid.begin(); cell != grid.end(); ++cell, ++index) {
        if (predicate(*cell)) {
            return Position{index / Columns, index % Columns};
        }
    }
    return std::nullopt;
}

// Agrega varios elementos al final, en orden. La fold expression sobre el
// operador coma repite el push_back para cada elemento del paquete (§6.4).
template<typename T, typename... Values>
void appendEvents(std::vector<T>& destination, Values&&... values) {
    (destination.push_back(std::forward<Values>(values)), ...);
}

// Combina varias lambdas en un solo objeto invocable, para usarlo con std::visit.
template<typename... Callables>
struct Overloaded : Callables... {
    using Callables::operator()...;
};

template<typename... Callables>
Overloaded(Callables...) -> Overloaded<Callables...>;
