//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <stdexcept>
#include <variant>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/position.hpp"

// CellTraits se verifica al compilar: plantilla general, especialización total
// (Wall) y parcial (ResourceCell con cualquier tipo de recompensa).
static_assert(CellTraits<Empty>::traversable && !CellTraits<Empty>::collectible);
static_assert(CellTraits<RoughTerrain>::traversable && !CellTraits<RoughTerrain>::collectible);
static_assert(CellTraits<Battery>::traversable && !CellTraits<Battery>::collectible);
static_assert(CellTraits<Trap>::traversable && !CellTraits<Trap>::collectible);
static_assert(CellTraits<Exit>::traversable && !CellTraits<Exit>::collectible);
static_assert(!CellTraits<Wall>::traversable && !CellTraits<Wall>::collectible);
static_assert(CellTraits<ResourceCell<int>>::traversable);
static_assert(CellTraits<ResourceCell<int>>::collectible);
static_assert(CellTraits<ResourceCell<double>>::collectible);

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

    void testCellDefaults() {
        const Cell cell{};
        assert(std::holds_alternative<Empty>(cell));

        const RoughTerrain rough{};
        assert(rough.energyCost == 2);

        const Battery battery{};
        assert(battery.energy == 3);
        assert(!battery.consumed);

        const Trap trap{};
        assert(trap.energyPenalty == 2);
        assert(trap.scorePenalty == 1);

        const ResourceCell<int> resource{10};
        assert(resource.reward == 10);
        assert(!resource.collected);
    }

    void testTraitsThroughCell() {
        assert(isTraversable(Cell{Empty{}}));
        assert(!isTraversable(Cell{Wall{}}));
        assert(isTraversable(Cell{RoughTerrain{}}));
        assert(isTraversable(Cell{ResourceCell<int>{10}}));
        assert(isTraversable(Cell{Battery{}}));
        assert(isTraversable(Cell{Trap{}}));
        assert(isTraversable(Cell{Exit{}}));

        assert(!isCollectible(Cell{Empty{}}));
        assert(!isCollectible(Cell{Wall{}}));
        assert(!isCollectible(Cell{RoughTerrain{}}));
        assert(isCollectible(Cell{ResourceCell<int>{10}}));
        assert(!isCollectible(Cell{Battery{}}));
        assert(!isCollectible(Cell{Trap{}}));
        assert(!isCollectible(Cell{Exit{}}));
    }

    void testCollectedResourceStillCounts() {
        Cell cell = ResourceCell<int>{10};
        std::get<ResourceCell<int>>(cell).collected = true;
        // El rasgo depende del tipo, no del estado: sirve para contar el total del mapa
        assert(isCollectible(cell));
        assert(isTraversable(cell));
    }
}

int main() {
    testPositionEquality();
    testNeighborFromInterior();
    testNeighborAtEdges();
    testNeighborIgnoresBoardSize();
    testNeighborRejectsUnknownAction();
    testToString();
    testCellDefaults();
    testTraitsThroughCell();
    testCollectedResourceStillCounts();
    return 0;
}
