// Pruebas de los templates propios (§6.1) y de la clase variádica Overloaded (§6.4)

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <list>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/algorithms.hpp"
#include "circuit_escape/cells.hpp"
#include "circuit_escape/grid.hpp"
#include "circuit_escape/position.hpp"

namespace {
    bool isEven(const int value) { return value % 2 == 0; }

    Grid<Cell, 3, 4> gridWithWallsAndExit() {
        Grid<Cell, 3, 4> grid{};
        const Position firstWall{0, 1};
        const Position secondWall{2, 2};
        const Position exit{2, 3};
        grid.at(firstWall) = Wall{};
        grid.at(secondWall) = Wall{};
        grid.at(exit) = Exit{};
        return grid;
    }

    void testCountMatchingOnEmptyRange() {
        const std::vector<int> numbers;
        assert(countMatching(numbers.begin(), numbers.end(), isEven) == 0);
    }

    void testCountMatchingOnVector() {
        const std::vector<int> numbers{1, 2, 3, 4, 5, 6};
        assert(countMatching(numbers.begin(), numbers.end(), isEven) == 3);
        assert(countMatching(numbers.begin(), numbers.end(), [](int value) { return value > 10; }) == 0);
    }

    // El mismo template con otro contenedor y otro tipo de dato (§6.1)
    void testCountMatchingOnList() {
        const std::list<Position> positions{Position{0, 0}, Position{0, 3}, Position{1, 2}};
        const std::size_t inFirstRow = countMatching(positions.begin(), positions.end(),
                                                     [](const Position& position) { return position.row == 0; });
        assert(inFirstRow == 2);
    }

    // Y con los iteradores de Grid
    void testCountMatchingOnGrid() {
        const Grid<Cell, 3, 4> grid = gridWithWallsAndExit();
        const std::size_t walls = countMatching(grid.begin(), grid.end(),
                                                [](const Cell& cell) { return std::holds_alternative<Wall>(cell); });
        const std::size_t exits = countMatching(grid.begin(), grid.end(),
                                                [](const Cell& cell) { return std::holds_alternative<Exit>(cell); });
        const std::size_t blocked = countMatching(grid.begin(), grid.end(),
                                                  [](const Cell& cell) { return !isTraversable(cell); });
        assert(walls == 2);
        assert(exits == 1);
        assert(blocked == 2);
    }

    void testMinimumByOnEmptyRange() {
        const std::vector<Action> actions;
        assert(!minimumBy(actions.begin(), actions.end(), [](Action) { return 0; }).has_value());
    }

    void testMinimumByChoosesLowestCost() {
        const std::vector<Action> actions{Action::up, Action::down, Action::left};
        // La acción más barata es down
        const auto cheapest = minimumBy(actions.begin(), actions.end(), [](const Action action) {
            if (action == Action::down) return 1;
            if (action == Action::left) return 5;
            return 9;
        });
        assert(cheapest == Action::down);
    }

    void testMinimumByKeepsFirstOnTies() {
        const std::list<int> numbers{4, 2, 7, 2};
        const auto smallest = minimumBy(numbers.begin(), numbers.end(), [](const int value) { return value; });
        assert(smallest == 2);
        // Con costos iguales gana el primero, para que la decisión sea reproducible
        const std::vector<std::string> words{"uno", "dos", "tres"};
        const auto firstWord = minimumBy(words.begin(), words.end(), [](const std::string&) { return 0; });
        assert(firstWord == "uno");
    }

    void testFindPositionLocatesTheExit() {
        const Grid<Cell, 3, 4> grid = gridWithWallsAndExit();
        const Position exit{2, 3};
        const auto found = findPosition(grid, [](const Cell& cell) { return std::holds_alternative<Exit>(cell); });
        assert(found == exit);
    }

    void testFindPositionReturnsNulloptWhenMissing() {
        const Grid<Cell, 3, 4> grid{};
        const auto found = findPosition(grid, [](const Cell& cell) { return std::holds_alternative<Exit>(cell); });
        assert(!found.has_value());
    }

    void testFindPositionReturnsTheFirstInRowOrder() {
        const Grid<Cell, 3, 4> grid = gridWithWallsAndExit();
        const Position firstWall{0, 1};
        const auto found = findPosition(grid, [](const Cell& cell) { return std::holds_alternative<Wall>(cell); });
        assert(found == firstWall);
    }

    void testAppendEventsKeepsOrder() {
        std::vector<std::string> events{"inicio"};
        appendEvents(events, std::string{"movimiento"}, std::string{"energia"}, std::string{"meta"});
        assert(events.size() == 4);
        assert(events[0] == "inicio");
        assert(events[1] == "movimiento");
        assert(events[2] == "energia");
        assert(events[3] == "meta");
    }

    void testAppendEventsWithoutValues() {
        std::vector<int> events{1, 2};
        appendEvents(events);
        assert(events.size() == 2);
    }

    void testOverloadedVisitsEveryAlternative() {
        const auto symbolOf = [](const Cell& cell) {
            return std::visit(Overloaded{
                                  [](const Empty&) { return '.'; },
                                  [](const Wall&) { return '#'; },
                                  [](const RoughTerrain&) { return '~'; },
                                  [](const ResourceCell<int>&) { return 'R'; },
                                  [](const Battery&) { return 'B'; },
                                  [](const Trap&) { return 'T'; },
                                  [](const Exit&) { return 'S'; },
                              },
                              cell);
        };
        assert(symbolOf(Cell{Empty{}}) == '.');
        assert(symbolOf(Cell{Wall{}}) == '#');
        assert(symbolOf(Cell{RoughTerrain{}}) == '~');
        assert(symbolOf(Cell{ResourceCell<int>{10}}) == 'R');
        assert(symbolOf(Cell{Battery{}}) == 'B');
        assert(symbolOf(Cell{Trap{}}) == 'T');
        assert(symbolOf(Cell{Exit{}}) == 'S');
    }
}

int main() {
    testCountMatchingOnEmptyRange();
    testCountMatchingOnVector();
    testCountMatchingOnList();
    testCountMatchingOnGrid();
    testMinimumByOnEmptyRange();
    testMinimumByChoosesLowestCost();
    testMinimumByKeepsFirstOnTies();
    testFindPositionLocatesTheExit();
    testFindPositionReturnsNulloptWhenMissing();
    testFindPositionReturnsTheFirstInRowOrder();
    testAppendEventsKeepsOrder();
    testAppendEventsWithoutValues();
    testOverloadedVisitsEveryAlternative();
    return 0;
}
