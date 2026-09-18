#include "circuit_escape/position.hpp"

#include <stdexcept>

std::optional<Position> neighbor(const Position origin, const Action action) {
    switch (action) {
        case Action::up:
            if (origin.row == 0) return std::nullopt;
            return Position{origin.row - 1, origin.column};
        case Action::down:
            return Position{origin.row + 1, origin.column};
        case Action::left:
            if (origin.column == 0) return std::nullopt;
            return Position{origin.row, origin.column - 1};
        case Action::right:
            return Position{origin.row, origin.column + 1};
        case Action::wait:
            return origin;
    }
    // Un enum class puede contener un valor fuera de la lista (static_cast)
    throw std::invalid_argument("neighbor: acción no válida");
}

std::size_t manhattanDistance(const Position from, const Position to) {
    // Las coordenadas no tienen signo: se resta siempre la menor de la mayor
    const std::size_t rows = from.row < to.row ? to.row - from.row : from.row - to.row;
    const std::size_t columns = from.column < to.column ? to.column - from.column : from.column - to.column;
    return rows + columns;
}

std::string toString(const Position position) {
    return "(" + std::to_string(position.row) + "," + std::to_string(position.column) + ")";
}
