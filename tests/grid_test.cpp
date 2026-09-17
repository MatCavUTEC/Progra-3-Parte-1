//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <stdexcept>

#include "circuit_escape/position.hpp"

namespace {
    void testPositionEquality() {
        const Position first{1, 2};
        const Position same{1, 2};
        const Position swapped{2, 1};
        const Position origin{0, 0};
        assert(first == same);
        assert(first != swapped);
        assert(Position{} == origin);
    }

    void testNeighborFromInterior() {
        const Position origin{2, 3};
        const Position above{1, 3};
        const Position below{3, 3};
        const Position toLeft{2, 2};
        const Position toRight{2, 4};
        assert(neighbor(origin, Action::up) == above);
        assert(neighbor(origin, Action::down) == below);
        assert(neighbor(origin, Action::left) == toLeft);
        assert(neighbor(origin, Action::right) == toRight);
        assert(neighbor(origin, Action::wait) == origin);
    }

    void testNeighborAtEdges() {
        const Position topRow{0, 3};
        assert(!neighbor(topRow, Action::up).has_value());
        assert(neighbor(topRow, Action::left).has_value());

        const Position leftColumn{2, 0};
        assert(!neighbor(leftColumn, Action::left).has_value());
        assert(neighbor(leftColumn, Action::up).has_value());

        const Position corner{0, 0};
        const Position below{1, 0};
        const Position toRight{0, 1};
        assert(!neighbor(corner, Action::up).has_value());
        assert(!neighbor(corner, Action::left).has_value());
        assert(neighbor(corner, Action::down) == below);
        assert(neighbor(corner, Action::right) == toRight);
        assert(neighbor(corner, Action::wait) == corner);
    }

    void testNeighborIgnoresBoardSize() {
        // Última celda de un tablero de 20 x 30: el límite superior lo valida Grid
        const Position last{19, 29};
        const Position below{20, 29};
        const Position toRight{19, 30};
        assert(neighbor(last, Action::down) == below);
        assert(neighbor(last, Action::right) == toRight);
    }

    void testNeighborRejectsUnknownAction() {
        bool thrown = false;
        try {
            static_cast<void>(neighbor(Position{}, static_cast<Action>(99)));
        } catch (const std::invalid_argument&) {
            thrown = true;
        }
        assert(thrown);
    }

    void testToString() {
        const Position position{10, 24};
        assert(toString(position) == "(10,24)");
        assert(toString(Position{}) == "(0,0)");
    }
}

int main() {
    testPositionEquality();
    testNeighborFromInterior();
    testNeighborAtEdges();
    testNeighborIgnoresBoardSize();
    testNeighborRejectsUnknownAction();
    testToString();
    return 0;
}
