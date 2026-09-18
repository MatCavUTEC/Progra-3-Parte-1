// Pruebas de los escenarios de demostración de 20 x 30 (§5.1, §10.2).
// Las pruebas sí leen archivos; el motor no.

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/algorithms.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"

namespace {
    using Environment = NavigationEnvironment<20, 30>;

    std::vector<std::string> readMap(const std::string& name) {
        const std::string path = std::string{CIRCUIT_ESCAPE_MAPS_DIR} + "/" + name;
        std::ifstream file{path};
        if (!file) {
            throw std::runtime_error("no se pudo abrir el mapa " + path);
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    Action actionFrom(const char symbol) {
        switch (symbol) {
            case 'U': return Action::up;
            case 'D': return Action::down;
            case 'L': return Action::left;
            case 'R': return Action::right;
            default: throw std::invalid_argument("acción desconocida en la ruta");
        }
    }

    std::size_t countCells(const Grid<Cell, 20, 30>& grid, bool (*matches)(const Cell&)) {
        return countMatching(grid.begin(), grid.end(), matches);
    }

    bool isWall(const Cell& cell) { return std::holds_alternative<Wall>(cell); }
    bool isExit(const Cell& cell) { return std::holds_alternative<Exit>(cell); }
    bool isResource(const Cell& cell) { return std::holds_alternative<ResourceCell<int>>(cell); }
    bool isBattery(const Cell& cell) { return std::holds_alternative<Battery>(cell); }
    bool isTrap(const Cell& cell) { return std::holds_alternative<Trap>(cell); }
    bool isRough(const Cell& cell) { return std::holds_alternative<RoughTerrain>(cell); }

    void checkShape(const std::string& name) {
        const Scenario<20, 30> scenario = parseScenario<20, 30>(readMap(name), rulesFor(Difficulty::standard));
        const Position start{1, 1};
        assert(scenario.start == start);
        assert(isTraversable(scenario.grid.at(scenario.start)));
        assert(countCells(scenario.grid, isExit) == 1);
        assert(countCells(scenario.grid, isResource) == 3);
        assert(countCells(scenario.grid, isBattery) == 2);
        assert(countCells(scenario.grid, isTrap) >= 3);
        assert(countCells(scenario.grid, isRough) > 0);

        // El borde de muros se conserva (§5.8)
        for (std::size_t column = 0; column < 30; ++column) {
            const Position top{0, column};
            const Position bottom{19, column};
            assert(isWall(scenario.grid.at(top)));
            assert(isWall(scenario.grid.at(bottom)));
        }
        for (std::size_t row = 0; row < 20; ++row) {
            const Position left{row, 0};
            const Position right{row, 29};
            assert(isWall(scenario.grid.at(left)));
            assert(isWall(scenario.grid.at(right)));
        }
    }

    // Reproduce una ruta fija y comprueba que la partida se completa
    void checkRoute(const std::string& name, const std::string& route) {
        const GameRules rules = rulesFor(Difficulty::standard);
        const Scenario<20, 30> scenario = parseScenario<20, 30>(readMap(name), rules);
        Environment environment{scenario.grid, scenario.start, rules};

        StepResult result;
        for (const char symbol : route) {
            assert(!environment.isFinished());
            result = environment.step(actionFrom(symbol));
        }

        assert(result.finished);
        assert(result.reason == EndReason::goalReached);
        assert(result.observation.agent == result.observation.goal);
        assert(result.observation.energy > 0);
        assert(result.observation.turn == route.size());
        assert(result.observation.turn < rules.turnLimit);
    }

    void testFirstScenario() {
        checkShape("scenario_01.txt");
        // Ruta por las puertas derechas y ruta por las izquierdas
        checkRoute("scenario_01.txt", "RRRRRRRRRRRRRRRRRRRRRRRDDDDDDRRDDDDDDRRDDDDD");
        checkRoute("scenario_01.txt", "RRDDDRRDDDRDDDRRRDDDRRRRRRRRRRRRRRRRRRRDDDDD");
    }

    void testSecondScenario() {
        checkShape("scenario_02.txt");
        // Por encima del bloque central y por debajo
        checkRoute("scenario_02.txt", "RRRRRRRRRRRRRRRRRRRRRRRRRRRDDDDDDDDDDDDDDDDD");
        checkRoute("scenario_02.txt", "RRRRRRRDDDDDDDDDDDDDRRRRRRRRRRRRRRRRRRRRDDDD");
    }
}

int main() {
    testFirstScenario();
    testSecondScenario();
    return 0;
}
