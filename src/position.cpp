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

std::string toString(const Position position) {
    return "(" + std::to_string(position.row) + "," + std::to_string(position.column) + ")";
}
