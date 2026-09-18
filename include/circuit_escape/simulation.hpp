#pragma once
#include <cstddef>
#include <vector>

#include "circuit_escape/controllers.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/events.hpp"

// Resultado de una partida jugada sin interfaz (§4, §7).
struct SimulationSummary {
    bool completed{false};
    EndReason reason{EndReason::none};
    std::size_t turns{};
    int energy{};
    int maximumEnergy{};
    std::size_t collectedResources{};
    int score{};

    friend bool operator==(const SimulationSummary&, const SimulationSummary&) = default;
};

// Resumen del estado actual, haya terminado la partida o no.
template<std::size_t Rows, std::size_t Columns>
[[nodiscard]] SimulationSummary summaryOf(const NavigationEnvironment<Rows, Columns>& environment) {
    const Observation observation = environment.state();
    // Llegar a la salida sin energía no es victoria: manda el motivo del término
    return SimulationSummary{.completed = environment.endReason() == EndReason::goalReached,
                             .reason = environment.endReason(),
                             .turns = observation.turn,
                             .energy = observation.energy,
                             .maximumEnergy = observation.maximumEnergy,
                             .collectedResources = observation.collectedResources,
                             .score = observation.score};
}

// Juega hasta que la partida termina. No imprime ni lee: quien llame decide
// qué hacer con el resultado.
template<std::size_t Rows, std::size_t Columns>
[[nodiscard]] SimulationSummary runSimulation(NavigationEnvironment<Rows, Columns>& environment,
                                              IController& controller) {
    while (!environment.isFinished()) {
        const std::vector<Action> legalActions = environment.availableActions();
        static_cast<void>(environment.step(controller.selectAction(environment.state(), legalActions)));
    }
    return summaryOf(environment);
}
