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
#include "circuit_escape/controllers.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"
#include "circuit_escape/simulation.hpp"

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

    Environment loadEnvironment(const std::string& name) {
        const GameRules rules = rulesFor(Difficulty::standard);
        const Scenario<20, 30> scenario = parseScenario<20, 30>(readMap(name), rules);
        return Environment{scenario.grid, scenario.start, rules};
    }

    // §10.2: la simulación automática con una semilla fija es reproducible
    void testSameSeedGivesTheSameSummary() {
        PolicyController<RandomPolicy> first{RandomPolicy{2026}};
        Environment firstEnvironment = loadEnvironment("scenario_01.txt");
        const SimulationSummary firstRun = runSimulation(firstEnvironment, first);

        PolicyController<RandomPolicy> second{RandomPolicy{2026}};
        Environment secondEnvironment = loadEnvironment("scenario_01.txt");
        const SimulationSummary secondRun = runSimulation(secondEnvironment, second);

        assert(firstRun == secondRun);
        assert(firstRun.turns > 0);
        assert(firstRun.reason != EndReason::none);
    }

    // La heurística resuelve los dos escenarios de demostración
    void testHeuristicCompletesBothScenarios() {
        for (const char* name : {"scenario_01.txt", "scenario_02.txt"}) {
            PolicyController<HeuristicPolicy> controller{HeuristicPolicy{}};
            Environment environment = loadEnvironment(name);
            const SimulationSummary summary = runSimulation(environment, controller);
            assert(summary.completed);
            assert(summary.reason == EndReason::goalReached);
            assert(summary.energy > 0);
            // Coincide con la ruta más corta comprobada para estos mapas
            assert(summary.turns == 44);
        }
    }

    // Llegar a la salida sin energía no cuenta como partida completada (§5.5)
    void testReachingTheExitWithoutEnergyIsNotCompleted() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 1;
        std::vector<std::string> lines;
        lines.push_back(std::string(30, '#'));
        lines.push_back("#@S" + std::string(26, '.') + "#");
        for (std::size_t row = 2; row < 19; ++row) {
            lines.push_back("#" + std::string(28, '.') + "#");
        }
        lines.push_back(std::string(30, '#'));

        const Scenario<20, 30> scenario = parseScenario<20, 30>(lines, rules);
        Environment environment{scenario.grid, scenario.start, rules};
        PolicyController<HeuristicPolicy> controller{HeuristicPolicy{}};
        const SimulationSummary summary = runSimulation(environment, controller);

        assert(summary.turns == 1);
        assert(summary.energy == 0);
        assert(summary.reason == EndReason::noEnergy);
        assert(!summary.completed);
        assert(environment.state().agent == environment.state().goal);
    }

    void testSimulationAlwaysEnds() {
        const GameRules rules = rulesFor(Difficulty::standard);
        PolicyController<RandomPolicy> controller{RandomPolicy{7}};
        Environment environment = loadEnvironment("scenario_02.txt");
        const SimulationSummary summary = runSimulation(environment, controller);
        assert(environment.isFinished());
        assert(summary.turns <= rules.turnLimit);
        assert(summary.energy >= 0);
        assert(summary.completed == (summary.reason == EndReason::goalReached));
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
    testSameSeedGivesTheSameSummary();
    testHeuristicCompletesBothScenarios();
    testReachingTheExitWithoutEnergyIsNotCompleted();
    testSimulationAlwaysEnds();
    return 0;
}
