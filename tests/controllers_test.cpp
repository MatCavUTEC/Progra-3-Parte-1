//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "circuit_escape/controllers.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/position.hpp"
#include "circuit_escape/scenario.hpp"

namespace {
    using Environment = NavigationEnvironment<3, 4>;

    // Política inválida: devuelve void en lugar de Action
    struct BadPolicy {
        void selectAction(const Observation&, std::span<const Action>) {}
    };

    Environment makeEnvironment(const std::vector<std::string>& lines, const GameRules& rules) {
        const Scenario<3, 4> scenario = parseScenario<3, 4>(lines, rules);
        return Environment{scenario.grid, scenario.start, rules};
    }

    Environment openEnvironment() {
        return makeEnvironment({"....", "@..S", "...."}, rulesFor(Difficulty::standard));
    }

    bool isLegal(const std::vector<Action>& legalActions, const Action action) {
        return std::find(legalActions.begin(), legalActions.end(), action) != legalActions.end();
    }

    // Juega una partida completa y devuelve las acciones elegidas
    std::vector<Action> playEpisode(Environment& environment, IController& controller) {
        std::vector<Action> chosen;
        while (!environment.isFinished()) {
            const std::vector<Action> legalActions = environment.availableActions();
            const Action action = controller.selectAction(environment.state(), legalActions);
            chosen.push_back(action);
            static_cast<void>(environment.step(action));
        }
        return chosen;
    }

    void testManhattanDistance() {
        const Position origin{1, 0};
        const Position goal{1, 3};
        const Position corner{2, 3};
        assert(manhattanDistance(origin, goal) == 3);
        assert(manhattanDistance(goal, origin) == 3);
        assert(manhattanDistance(origin, corner) == 4);
        assert(manhattanDistance(goal, goal) == 0);
    }

    void testRandomPolicyChoosesLegalActions() {
        Environment environment = openEnvironment();
        RandomPolicy policy{123};
        for (int turn = 0; turn < 30 && !environment.isFinished(); ++turn) {
            const std::vector<Action> legalActions = environment.availableActions();
            const Action action = policy.selectAction(environment.state(), legalActions);
            assert(isLegal(legalActions, action));
            static_cast<void>(environment.step(action));
        }
    }

    void testRandomPolicyWithASingleAction() {
        Environment environment = openEnvironment();
        RandomPolicy policy{5};
        const std::vector<Action> onlyWait{Action::wait};
        for (int attempt = 0; attempt < 10; ++attempt) {
            assert(policy.selectAction(environment.state(), onlyWait) == Action::wait);
        }
    }

    void testRandomPolicyRejectsEmptyActions() {
        const Environment environment = openEnvironment();
        RandomPolicy policy{1};
        const std::vector<Action> none;
        bool thrown = false;
        try {
            static_cast<void>(policy.selectAction(environment.state(), none));
        } catch (const std::invalid_argument&) {
            thrown = true;
        }
        assert(thrown);
    }

    // §8: la misma semilla reproduce la simulación
    void testSameSeedRepeatsTheEpisode() {
        Environment first = openEnvironment();
        PolicyController<RandomPolicy> firstController{RandomPolicy{42}};
        const std::vector<Action> firstRun = playEpisode(first, firstController);

        Environment second = openEnvironment();
        PolicyController<RandomPolicy> secondController{RandomPolicy{42}};
        const std::vector<Action> secondRun = playEpisode(second, secondController);

        assert(firstRun == secondRun);
        assert(first.state().agent == second.state().agent);
        assert(first.state().energy == second.state().energy);
        assert(first.state().turn == second.state().turn);
        assert(first.state().score == second.state().score);
    }

    void testHeuristicMovesTowardTheGoal() {
        const Environment environment = openEnvironment();
        HeuristicPolicy policy;
        const std::vector<Action> legalActions = environment.availableActions();
        assert(policy.selectAction(environment.state(), legalActions) == Action::right);
    }

    void testHeuristicPrefersMovingOverWaiting() {
        // Con un muro a la derecha ningún movimiento acerca a la salida,
        // pero esperar tampoco: la política se mueve igual
        const Environment environment = makeEnvironment({"....", "@#.S", "...."},
                                                        rulesFor(Difficulty::standard));
        HeuristicPolicy policy;
        const std::vector<Action> legalActions = environment.availableActions();
        const Action action = policy.selectAction(environment.state(), legalActions);
        assert(action != Action::wait);
        assert(isLegal(legalActions, action));
    }

    void testHeuristicWaitsOnlyWhenTrapped() {
        HeuristicPolicy policy;
        const Environment environment = openEnvironment();
        const std::vector<Action> onlyWait{Action::wait};
        assert(policy.selectAction(environment.state(), onlyWait) == Action::wait);
    }

    void testHeuristicReachesTheGoal() {
        Environment environment = openEnvironment();
        PolicyController<HeuristicPolicy> controller{HeuristicPolicy{}};
        static_cast<void>(playEpisode(environment, controller));
        assert(environment.isFinished());
        assert(environment.state().agent == environment.state().goal);
    }

    void testHumanPolicyUsesThePendingAction() {
        const Environment environment = openEnvironment();
        HumanPolicy policy;
        policy.setNextAction(Action::down);
        const std::vector<Action> legalActions = environment.availableActions();
        assert(policy.selectAction(environment.state(), legalActions) == Action::down);

        // La acción se consume: sin una nueva decisión no hay nada que entregar
        bool thrown = false;
        try {
            static_cast<void>(policy.selectAction(environment.state(), legalActions));
        } catch (const std::invalid_argument&) {
            thrown = true;
        }
        assert(thrown);
    }

    // §6.5: el entorno no sabe qué controlador lo maneja; el despacho es dinámico
    void testControllersAreInterchangeable() {
        HumanPolicy human;
        human.setNextAction(Action::wait);

        std::vector<std::unique_ptr<IController>> controllers;
        controllers.push_back(std::make_unique<PolicyController<RandomPolicy>>(RandomPolicy{7}));
        controllers.push_back(std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{}));
        controllers.push_back(std::make_unique<PolicyController<HumanPolicy>>(human));

        Environment environment = openEnvironment();
        const std::vector<Action> legalActions = environment.availableActions();
        for (const std::unique_ptr<IController>& controller : controllers) {
            const Action action = controller->selectAction(environment.state(), legalActions);
            assert(isLegal(legalActions, action));
        }

        // El mismo entorno se juega cambiando de controlador entre turnos
        std::size_t index = 0;
        while (!environment.isFinished()) {
            IController& controller = *controllers[index % controllers.size()];
            const std::vector<Action> actions = environment.availableActions();
            if (index % controllers.size() == 2) {
                static_cast<void>(environment.step(Action::wait));  // el humano ya gastó su acción
            } else {
                static_cast<void>(environment.step(controller.selectAction(environment.state(), actions)));
            }
            ++index;
        }
        assert(environment.isFinished());
    }
}

// El concept se comprueba al compilar
static_assert(NavigationPolicy<RandomPolicy>);
static_assert(NavigationPolicy<HeuristicPolicy>);
static_assert(NavigationPolicy<HumanPolicy>);
static_assert(!NavigationPolicy<BadPolicy>);

int main() {
    testManhattanDistance();
    testRandomPolicyChoosesLegalActions();
    testRandomPolicyWithASingleAction();
    testRandomPolicyRejectsEmptyActions();
    testSameSeedRepeatsTheEpisode();
    testHeuristicMovesTowardTheGoal();
    testHeuristicPrefersMovingOverWaiting();
    testHeuristicWaitsOnlyWhenTrapped();
    testHeuristicReachesTheGoal();
    testHumanPolicyUsesThePendingAction();
    testControllersAreInterchangeable();
    return 0;
}
