//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/algorithms.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"

namespace {
    using Environment = NavigationEnvironment<3, 4>;

    Environment makeEnvironment(const std::vector<std::string>& lines, const GameRules& rules) {
        const Scenario<3, 4> scenario = parseScenario<3, 4>(lines, rules);
        return Environment{scenario.grid, scenario.start, rules};
    }

    template<typename Event>
    std::size_t countEvents(const std::vector<NavigationEvent>& events) {
        return countMatching(events.begin(), events.end(), [](const NavigationEvent& event) {
            return std::holds_alternative<Event>(event);
        });
    }

    void testStandardProfile() {
        const GameRules rules = rulesFor(Difficulty::standard);
        assert(rules.initialEnergy == 60);
        assert(rules.turnLimit == 180);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 2);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 10);
        assert(rules.batteryEnergy == 3);
        assert(rules.trapEnergyPenalty == 2);
        assert(rules.trapScorePenalty == 1);
    }

    void testEasyProfile() {
        const GameRules rules = rulesFor(Difficulty::easy);
        assert(rules.initialEnergy == 80);
        assert(rules.turnLimit == 240);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 2);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 15);
        assert(rules.batteryEnergy == 5);
        assert(rules.trapEnergyPenalty == 1);
        assert(rules.trapScorePenalty == 0);
    }

    void testHardProfile() {
        const GameRules rules = rulesFor(Difficulty::hard);
        assert(rules.initialEnergy == 40);
        assert(rules.turnLimit == 140);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 3);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 8);
        assert(rules.batteryEnergy == 2);
        assert(rules.trapEnergyPenalty == 3);
        assert(rules.trapScorePenalty == 2);
    }

    void testDefaultRulesAreStandard() {
        // Los valores por defecto de GameRules y de las celdas son los del perfil standard
        assert(GameRules{} == rulesFor(Difficulty::standard));
    }

    void testProfilesDifferFromEachOther() {
        assert(!(rulesFor(Difficulty::easy) == rulesFor(Difficulty::standard)));
        assert(!(rulesFor(Difficulty::hard) == rulesFor(Difficulty::standard)));
    }

    void testParseDifficulty() {
        assert(parseDifficulty("easy") == Difficulty::easy);
        assert(parseDifficulty("standard") == Difficulty::standard);
        assert(parseDifficulty("hard") == Difficulty::hard);
    }

    void testResourceIsCollectedOnce() {
        Environment environment = makeEnvironment({"@R.S", "....", "...."}, rulesFor(Difficulty::standard));
        const StepResult first = environment.step(Action::right);
        assert(first.observation.score == 10);
        assert(first.observation.collectedResources == 1);
        assert(first.observation.energy == 59);
        assert(countEvents<ResourceCollectedEvent>(first.events) == 1);

        static_cast<void>(environment.step(Action::left));
        const StepResult again = environment.step(Action::right);
        assert(again.observation.score == 10);
        assert(again.observation.collectedResources == 1);
        assert(countEvents<ResourceCollectedEvent>(again.events) == 0);
        assert(again.observation.energy == 57);
    }

    void testEventOrderWhenCollecting() {
        Environment environment = makeEnvironment({"@R.S", "....", "...."}, rulesFor(Difficulty::standard));
        const StepResult result = environment.step(Action::right);
        assert(result.events.size() == 3);
        assert(std::holds_alternative<MovedEvent>(result.events[0]));
        assert(std::holds_alternative<EnergyChangedEvent>(result.events[1]));
        assert(std::holds_alternative<ResourceCollectedEvent>(result.events[2]));
        const ResourceCollectedEvent collected = std::get<ResourceCollectedEvent>(result.events[2]);
        const Position resource{0, 1};
        assert(collected.at == resource);
        assert(collected.points == 10);
    }

    void testBatteryIsConsumedOnce() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 20;
        // Se gasta energía en el terreno elevado para que la recarga no quede recortada
        Environment environment = makeEnvironment({"@~~B", "....", "...S"}, rules);
        static_cast<void>(environment.step(Action::right));
        const StepResult beforeBattery = environment.step(Action::right);
        assert(beforeBattery.observation.energy == 16);

        // Cuesta 1 entrar y la batería devuelve 3
        const StepResult first = environment.step(Action::right);
        assert(first.observation.energy == 18);

        // Al volver a entrar la batería ya está consumida: solo se paga la entrada
        static_cast<void>(environment.step(Action::left));
        const StepResult again = environment.step(Action::right);
        assert(again.observation.energy == 15);
    }

    void testBatteryRespectsMaximumEnergy() {
        Environment environment = makeEnvironment({"@B.S", "....", "...."}, rulesFor(Difficulty::standard));
        const StepResult result = environment.step(Action::right);
        // 60 - 1 + 3 se recorta a la energía máxima
        assert(result.observation.energy == 60);
        assert(result.observation.maximumEnergy == 60);
    }

    // §5.5: el efecto se aplica aunque el costo de entrada haya dejado la energía en cero
    void testBatteryRecoversAfterEntryCost() {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = 3;
        Environment environment = makeEnvironment({"@~B.", "....", "...S"}, rules);
        const StepResult rough = environment.step(Action::right);
        assert(rough.observation.energy == 1);
        assert(!rough.finished);

        const StepResult battery = environment.step(Action::right);
        assert(battery.observation.energy == 3);
        assert(!battery.finished);
        assert(battery.reason == EndReason::none);
    }

    void testTrapAppliesBothPenaltiesEveryTime() {
        Environment environment = makeEnvironment({"@T.S", "....", "...."}, rulesFor(Difficulty::standard));
        const StepResult first = environment.step(Action::right);
        assert(first.observation.energy == 60 - 1 - 2);
        assert(first.observation.score == -1);
        assert(countEvents<TrapTriggeredEvent>(first.events) == 1);

        static_cast<void>(environment.step(Action::left));
        const StepResult again = environment.step(Action::right);
        assert(again.observation.energy == 57 - 1 - 1 - 2);
        assert(again.observation.score == -2);
        assert(countEvents<TrapTriggeredEvent>(again.events) == 1);
    }

    void testWaitDoesNotActivateTheCurrentCell() {
        Environment environment = makeEnvironment({"@T.S", "....", "...."}, rulesFor(Difficulty::standard));
        static_cast<void>(environment.step(Action::right));
        const StepResult waiting = environment.step(Action::wait);
        assert(waiting.observation.energy == 57 - 1);
        assert(waiting.observation.score == -1);
        assert(countEvents<TrapTriggeredEvent>(waiting.events) == 0);
    }

    void testEffectsUseTheProfileValues() {
        Environment easyResource = makeEnvironment({"@R.S", "....", "...."}, rulesFor(Difficulty::easy));
        const StepResult collected = easyResource.step(Action::right);
        assert(collected.observation.score == 15);

        Environment easyTrap = makeEnvironment({"@T.S", "....", "...."}, rulesFor(Difficulty::easy));
        const StepResult trapped = easyTrap.step(Action::right);
        assert(trapped.observation.energy == 80 - 1 - 1);
        assert(trapped.observation.score == 0);
    }

    void testResetRestoresConsumedCells() {
        Environment environment = makeEnvironment({"@R.S", "....", "...."}, rulesFor(Difficulty::standard));
        static_cast<void>(environment.step(Action::right));
        environment.reset(0);
        assert(environment.state().score == 0);
        assert(environment.state().collectedResources == 0);

        const StepResult result = environment.step(Action::right);
        assert(result.observation.score == 10);
        assert(result.observation.collectedResources == 1);
        assert(countEvents<ResourceCollectedEvent>(result.events) == 1);
    }

    void testReachingTheExit() {
        Environment environment = makeEnvironment({"@RBS", "....", "...."}, rulesFor(Difficulty::standard));
        static_cast<void>(environment.step(Action::right));
        static_cast<void>(environment.step(Action::right));
        const StepResult result = environment.step(Action::right);
        assert(result.finished);
        assert(result.reason == EndReason::goalReached);
        assert(countEvents<GoalReachedEvent>(result.events) == 1);
        assert(result.observation.score == 10);
        assert(result.observation.collectedResources == 1);
    }

    // §8: cada tipo de evento debe poder procesarse
    void testEveryEventTypeCanBeProcessed() {
        const Position at{1, 2};
        const std::vector<NavigationEvent> events{
            MovedEvent{at, at, 1},
            MovementRejectedEvent{at, Action::up},
            ResourceCollectedEvent{at, 10},
            EnergyChangedEvent{5, 4},
            TrapTriggeredEvent{at},
            GoalReachedEvent{at},
        };

        std::vector<std::string> names;
        for (const NavigationEvent& event : events) {
            names.push_back(std::visit(Overloaded{
                                           [](const MovedEvent&) { return std::string{"moved"}; },
                                           [](const MovementRejectedEvent&) { return std::string{"rejected"}; },
                                           [](const ResourceCollectedEvent&) { return std::string{"resource"}; },
                                           [](const EnergyChangedEvent&) { return std::string{"energy"}; },
                                           [](const TrapTriggeredEvent&) { return std::string{"trap"}; },
                                           [](const GoalReachedEvent&) { return std::string{"goal"}; },
                                       },
                                       event));
        }

        const std::vector<std::string> expected{"moved", "rejected", "resource", "energy", "trap", "goal"};
        assert(names == expected);
    }

    void testParseDifficultyRejectsUnknownText() {
        assert(!parseDifficulty("medium").has_value());
        assert(!parseDifficulty("").has_value());
        // La opción de línea de comandos se escribe en minúsculas
        assert(!parseDifficulty("Easy").has_value());
    }
}

int main() {
    testStandardProfile();
    testEasyProfile();
    testHardProfile();
    testDefaultRulesAreStandard();
    testProfilesDifferFromEachOther();
    testParseDifficulty();
    testParseDifficultyRejectsUnknownText();
    testResourceIsCollectedOnce();
    testEventOrderWhenCollecting();
    testBatteryIsConsumedOnce();
    testBatteryRespectsMaximumEnergy();
    testBatteryRecoversAfterEntryCost();
    testTrapAppliesBothPenaltiesEveryTime();
    testWaitDoesNotActivateTheCurrentCell();
    testEffectsUseTheProfileValues();
    testResetRestoresConsumedCells();
    testReachingTheExit();
    testEveryEventTypeCanBeProcessed();
    return 0;
}
