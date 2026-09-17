//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <variant>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/grid.hpp"
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

    void testGridDimensionsAreCompileTime() {
        static_assert(Grid<Cell, 3, 4>::rows() == 3);
        static_assert(Grid<Cell, 3, 4>::columns() == 4);
        const Grid<Cell, 3, 4> grid{};
        assert(grid.rows() == 3);
        assert(grid.columns() == 4);
    }

    void testGridContains() {
        const Grid<Cell, 3, 4> grid{};
        const Position inside{2, 3};
        const Position rowOutside{3, 0};
        const Position columnOutside{0, 4};
        const Position farAway{100, 100};
        assert(grid.contains(Position{}));
        assert(grid.contains(inside));
        assert(!grid.contains(rowOutside));
        assert(!grid.contains(columnOutside));
        assert(!grid.contains(farAway));
    }

    void testGridStartsEmpty() {
        const Grid<Cell, 3, 4> cells{};
        assert(std::holds_alternative<Empty>(cells.at(Position{})));
        const Grid<int, 2, 2> numbers{};
        assert(numbers.at(Position{}) == 0);
    }

    void testGridStoresCellsByPosition() {
        Grid<Cell, 3, 4> grid{};
        const Position wallAt{1, 2};
        const Position untouched{1, 3};
        grid.at(wallAt) = Wall{};
        assert(std::holds_alternative<Wall>(grid.at(wallAt)));
        assert(std::holds_alternative<Empty>(grid.at(untouched)));
    }

    void testGridCornersAreIndependent() {
        Grid<Cell, 3, 4> grid{};
        const Position topLeft{0, 0};
        const Position topRight{0, 3};
        const Position bottomLeft{2, 0};
        const Position bottomRight{2, 3};
        grid.at(topLeft) = Wall{};
        grid.at(topRight) = Battery{};
        grid.at(bottomLeft) = Trap{};
        grid.at(bottomRight) = Exit{};
        assert(std::holds_alternative<Wall>(grid.at(topLeft)));
        assert(std::holds_alternative<Battery>(grid.at(topRight)));
        assert(std::holds_alternative<Trap>(grid.at(bottomLeft)));
        assert(std::holds_alternative<Exit>(grid.at(bottomRight)));
    }

    void testGridRejectsPositionsOutside() {
        Grid<Cell, 3, 4> grid{};
        const Position rowOutside{3, 0};
        const Position columnOutside{0, 4};
        const Position bothOutside{3, 4};
        int thrown = 0;
        for (const Position& position : {rowOutside, columnOutside, bothOutside}) {
            try {
                static_cast<void>(grid.at(position));
            } catch (const std::out_of_range&) {
                ++thrown;
            }
        }
        assert(thrown == 3);
    }

    void testGridConstAccess() {
        Grid<Cell, 3, 4> grid{};
        const Position batteryAt{1, 1};
        const Position outside{3, 4};
        grid.at(batteryAt) = Battery{};

        const Grid<Cell, 3, 4>& constGrid = grid;
        assert(std::holds_alternative<Battery>(constGrid.at(batteryAt)));

        bool thrown = false;
        try {
            static_cast<void>(constGrid.at(outside));
        } catch (const std::out_of_range&) {
            thrown = true;
        }
        assert(thrown);
    }

    void testGridIterationIsRowMajor() {
        Grid<int, 3, 4> grid{};
        int next = 0;
        for (std::size_t row = 0; row < grid.rows(); ++row) {
            for (std::size_t column = 0; column < grid.columns(); ++column) {
                const Position position{row, column};
                grid.at(position) = next;
                ++next;
            }
        }

        assert(std::distance(grid.begin(), grid.end()) == 12);
        int expected = 0;
        for (const int value : grid) {
            assert(value == expected);
            ++expected;
        }
        assert(expected == 12);
    }

    void testGridIteratorsCanModify() {
        Grid<int, 2, 2> grid{};
        for (int& value : grid) {
            value = 7;
        }
        const Position last{1, 1};
        assert(grid.at(last) == 7);
    }

    void testGridConstIterators() {
        Grid<Cell, 3, 4> grid{};
        const Position trapAt{2, 1};
        grid.at(trapAt) = Trap{};

        const Grid<Cell, 3, 4>& constGrid = grid;
        assert(std::distance(constGrid.begin(), constGrid.end()) == 12);
        assert(std::distance(grid.cbegin(), grid.cend()) == 12);

        const auto traps = std::count_if(constGrid.begin(), constGrid.end(),
                                         [](const Cell& cell) { return std::holds_alternative<Trap>(cell); });
        assert(traps == 1);
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
    testGridDimensionsAreCompileTime();
    testGridContains();
    testGridStartsEmpty();
    testGridStoresCellsByPosition();
    testGridCornersAreIndependent();
    testGridRejectsPositionsOutside();
    testGridConstAccess();
    testGridIterationIsRowMajor();
    testGridIteratorsCanModify();
    testGridConstIterators();
    return 0;
}
