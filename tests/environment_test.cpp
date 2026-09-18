//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"

namespace {
    using Environment = NavigationEnvironment<3, 4>;

    // Mapa base: el agente empieza en (1,0), la salida está en (1,3),
    // hay un muro en (1,1) y terreno elevado en (0,1).
    std::vector<std::string> baseMap() {
        return {
            ".~..",
            "@#.S",
            "....",
        };
    }

    Environment makeEnvironment(const std::vector<std::string>& lines, const GameRules& rules) {
        const Scenario<3, 4> scenario = parseScenario<3, 4>(lines, rules);
        return Environment{scenario.grid, scenario.start, rules};
    }

    Environment standardEnvironment() {
        return makeEnvironment(baseMap(), rulesFor(Difficulty::standard));
    }

    bool rejectsConstruction(const std::vector<std::string>& lines, const GameRules& rules) {
        try {
            static_cast<void>(makeEnvironment(lines, rules));
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }

    bool contains(const std::vector<Action>& actions, const Action action) {
        return std::find(actions.begin(), actions.end(), action) != actions.end();
    }

    template<typename Event>
    bool hasEvent(const std::vector<NavigationEvent>& events) {
        return std::any_of(events.begin(), events.end(), [](const NavigationEvent& event) {
            return std::holds_alternative<Event>(event);
        });
    }

    void testInitialState() {
        const Environment environment = standardEnvironment();
        const Observation observation = environment.state();
        const Position start{1, 0};
        const Position goal{1, 3};
        assert(observation.agent == start);
        assert(observation.goal == goal);
        assert(observation.energy == 60);
        assert(observation.maximumEnergy == 60);
        assert(observation.score == 0);
        assert(observation.collectedResources == 0);
        assert(observation.turn == 0);
        assert(!environment.isFinished());
    }

    void testConstructorPreconditions() {
        const GameRules rules = rulesFor(Difficulty::standard);
        // Sin salida
        assert(rejectsConstruction({".~..", "@#..", "...."}, rules));
        // Dos salidas
        assert(rejectsConstruction({".~.S", "@#.S", "...."}, rules));
        // El agente empieza sobre un muro no puede ocurrir: '@' deja la celda libre,
        // así que se construye el tablero a mano para probar esa precondición.
        Scenario<3, 4> scenario = parseScenario<3, 4>(baseMap(), rules);
        const Position start{1, 0};
        scenario.grid.at(start) = Wall{};
        bool rejected = false;
        try {
            static_cast<void>(Environment{scenario.grid, start, rules});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        assert(rejected);

        // Inicio fuera del tablero
        const Scenario<3, 4> valid = parseScenario<3, 4>(baseMap(), rules);
        const Position outside{3, 0};
        rejected = false;
        try {
            static_cast<void>(Environment{valid.grid, outside, rules});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        assert(rejected);

        // Energía inicial y límite de turnos deben ser positivos
        GameRules withoutEnergy = rules;
        withoutEnergy.initialEnergy = 0;
        assert(rejectsConstruction(baseMap(), withoutEnergy));
        GameRules withoutTurns = rules;
        withoutTurns.turnLimit = 0;
        assert(rejectsConstruction(baseMap(), withoutTurns));
    }

    void testFreeMove() {
        Environment environment = standardEnvironment();
        const StepResult result = environment.step(Action::down);
        const Position expected{2, 0};
        assert(result.observation.agent == expected);
        assert(result.observation.energy == 59);
        assert(result.observation.turn == 1);
        assert(!result.finished);
        assert(result.reason == EndReason::none);
        assert(hasEvent<MovedEvent>(result.events));
        assert(hasEvent<EnergyChangedEvent>(result.events));
        // El evento del movimiento va antes que el de la energía
        assert(std::holds_alternative<MovedEvent>(result.events.front()));
        const MovedEvent moved = std::get<MovedEvent>(result.events.front());
        const Position start{1, 0};
        assert(moved.from == start);
        assert(moved.to == expected);
        assert(moved.energyCost == 1);
    }

    void testMoveAgainstWall() {
        Environment environment = standardEnvironment();
        const StepResult result = environment.step(Action::right);
        const Position start{1, 0};
        assert(result.observation.agent == start);
        assert(result.observation.energy == 59);
        assert(result.observation.turn == 1);
        assert(std::holds_alternative<MovementRejectedEvent>(result.events.front()));
        assert(hasEvent<EnergyChangedEvent>(result.events));
        assert(!hasEvent<MovedEvent>(result.events));
    }

    void testMoveOutsideBoard() {
        Environment environment = standardEnvironment();
        const StepResult result = environment.step(Action::left);
        const Position start{1, 0};
        assert(result.observation.agent == start);
        assert(result.observation.energy == 59);
        assert(result.observation.turn == 1);
        assert(hasEvent<MovementRejectedEvent>(result.events));
    }

    void testWait() {
        Environment environment = standardEnvironment();
        const StepResult result = environment.step(Action::wait);
        const Position start{1, 0};
        assert(result.observation.agent == start);
        assert(result.observation.energy == 59);
        assert(result.observation.turn == 1);
        assert(!hasEvent<MovedEvent>(result.events));
        assert(!hasEvent<MovementRejectedEvent>(result.events));
        assert(hasEvent<EnergyChangedEvent>(result.events));
    }

    void testRoughTerrainCostsMore() {
        Environment environment = standardEnvironment();
        static_cast<void>(environment.step(Action::up));     // (0,0), cuesta 1
        const StepResult result = environment.step(Action::right);  // (0,1) es terreno elevado
        const Position rough{0, 1};
        assert(result.observation.agent == rough);
        assert(result.observation.energy == 57);
        const MovedEvent moved = std::get<MovedEvent>(result.events.front());
        assert(moved.energyCost == 2);
    }

    void testAvailableActionsNextToWall() {
        const Environment environment = standardEnvironment();
        const std::vector<Action> actions = environment.availableActions();
        // Desde (1,0): derecha es muro y izquierda sale del tablero
        assert(contains(actions, Action::wait));
        assert(contains(actions, Action::up));
        assert(contains(actions, Action::down));
        assert(!contains(actions, Action::right));
        assert(!contains(actions, Action::left));
        assert(environment.state().availableActions.size() == actions.size());
    }

    void testAvailableActionsInCorner() {
        const GameRules rules = rulesFor(Difficulty::standard);
        const Environment environment = makeEnvironment({"@...", "..#S", "...."}, rules);
        const std::vector<Action> actions = environment.availableActions();
        assert(contains(actions, Action::wait));
        assert(contains(actions, Action::right));
        assert(contains(actions, Action::down));
        assert(!contains(actions, Action::up));
        assert(!contains(actions, Action::left));
        assert(actions.size() == 3);
    }

    void testAvailableActionsAfterTheEnd() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 1;
        Environment environment = makeEnvironment(baseMap(), rules);
        const StepResult result = environment.step(Action::wait);
        assert(result.finished);
        assert(environment.availableActions().empty());
        assert(result.observation.availableActions.empty());
    }

    void testStepAfterTheEndThrows() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 1;
        Environment environment = makeEnvironment(baseMap(), rules);
        static_cast<void>(environment.step(Action::wait));
        bool thrown = false;
        try {
            static_cast<void>(environment.step(Action::wait));
        } catch (const std::logic_error&) {
            thrown = true;
        }
        assert(thrown);
    }

    void testEndByEnergy() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 2;
        Environment environment = makeEnvironment(baseMap(), rules);
        static_cast<void>(environment.step(Action::wait));
        const StepResult result = environment.step(Action::wait);
        assert(result.observation.energy == 0);
        assert(result.finished);
        assert(result.reason == EndReason::noEnergy);
        assert(environment.isFinished());
    }

    void testEndByTurnLimit() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.turnLimit = 2;
        Environment environment = makeEnvironment(baseMap(), rules);
        static_cast<void>(environment.step(Action::wait));
        const StepResult result = environment.step(Action::wait);
        assert(result.observation.turn == 2);
        assert(result.observation.energy > 0);
        assert(result.finished);
        assert(result.reason == EndReason::turnLimit);
    }

    void testReachingTheGoal() {
        const GameRules rules = rulesFor(Difficulty::standard);
        Environment environment = makeEnvironment({"....", "@..S", "...."}, rules);
        static_cast<void>(environment.step(Action::right));
        static_cast<void>(environment.step(Action::right));
        const StepResult result = environment.step(Action::right);
        const Position goal{1, 3};
        assert(result.observation.agent == goal);
        assert(result.finished);
        assert(result.reason == EndReason::goalReached);
        assert(hasEvent<GoalReachedEvent>(result.events));
    }

    // Precedencia: llegar a la salida sin energía no es victoria
    void testGoalWithoutEnergyIsNotAWin() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 1;
        Environment environment = makeEnvironment({"@S..", "....", "...."}, rules);
        const StepResult result = environment.step(Action::right);
        const Position goal{0, 1};
        assert(result.observation.agent == goal);
        assert(result.observation.energy == 0);
        assert(result.finished);
        assert(result.reason == EndReason::noEnergy);
        assert(!hasEvent<GoalReachedEvent>(result.events));
    }

    // Precedencia: llegar con energía en el último turno sí es victoria
    void testGoalOnTheLastTurnIsAWin() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.turnLimit = 1;
        Environment environment = makeEnvironment({"@S..", "....", "...."}, rules);
        const StepResult result = environment.step(Action::right);
        assert(result.observation.turn == 1);
        assert(result.observation.energy > 0);
        assert(result.finished);
        assert(result.reason == EndReason::goalReached);
    }

    // Precedencia: sin energía y con el límite de turnos a la vez gana noEnergy
    void testNoEnergyBeforeTurnLimit() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 1;
        rules.turnLimit = 1;
        Environment environment = makeEnvironment(baseMap(), rules);
        const StepResult result = environment.step(Action::wait);
        assert(result.observation.energy == 0);
        assert(result.observation.turn == 1);
        assert(result.reason == EndReason::noEnergy);
    }

    void testReset() {
        Environment environment = standardEnvironment();
        static_cast<void>(environment.step(Action::down));
        static_cast<void>(environment.step(Action::up));
        environment.reset(7);
        assert(environment.seed() == 7);

        const Observation observation = environment.state();
        const Position start{1, 0};
        assert(observation.agent == start);
        assert(observation.energy == 60);
        assert(observation.turn == 0);
        assert(observation.score == 0);
        assert(!environment.isFinished());
        // Después de reset se puede volver a jugar
        const StepResult result = environment.step(Action::down);
        assert(result.observation.turn == 1);
    }

    void testGridStaysReadable() {
        const Environment environment = standardEnvironment();
        const Position wall{1, 1};
        assert(std::holds_alternative<Wall>(environment.grid().at(wall)));
    }
}

int main() {
    testInitialState();
    testConstructorPreconditions();
    testFreeMove();
    testMoveAgainstWall();
    testMoveOutsideBoard();
    testWait();
    testRoughTerrainCostsMore();
    testAvailableActionsNextToWall();
    testAvailableActionsInCorner();
    testAvailableActionsAfterTheEnd();
    testStepAfterTheEndThrows();
    testEndByEnergy();
    testEndByTurnLimit();
    testReachingTheGoal();
    testGoalWithoutEnergyIsNotAWin();
    testGoalOnTheLastTurnIsAWin();
    testNoEnergyBeforeTurnLimit();
    testReset();
    testGridStaysReadable();
    return 0;
}
