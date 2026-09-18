//
// Created by LucasMCgamer on 13/09/2026.
//

#include "circuit_escape/controllers.hpp"

#include <stdexcept>
#include <vector>

#include "circuit_escape/algorithms.hpp"

namespace {
    void requireActions(const std::span<const Action> legalActions) {
        if (legalActions.empty()) {
            throw std::invalid_argument("selectAction: no hay acciones legales");
        }
    }
}

RandomPolicy::RandomPolicy(const std::uint32_t seed) : generator_(seed) {}

Action RandomPolicy::selectAction(const Observation&, const std::span<const Action> legalActions) {
    requireActions(legalActions);
    std::uniform_int_distribution<std::size_t> pick(0, legalActions.size() - 1);
    return legalActions[pick(generator_)];
}

Action HeuristicPolicy::selectAction(const Observation& observation,
                                     const std::span<const Action> legalActions) {
    requireActions(legalActions);

    // Esperar no acerca a la salida: solo se elige si no hay ningún movimiento
    std::vector<Action> moves;
    for (const Action action : legalActions) {
        if (action != Action::wait) {
            moves.push_back(action);
        }
    }
    if (moves.empty()) {
        return legalActions.front();
    }

    // Empates a favor del primero, para que la partida sea reproducible
    const std::optional<Action> best = minimumBy(moves.begin(), moves.end(), [&observation](const Action action) {
        return manhattanDistance(neighbor(observation.agent, action).value(), observation.goal);
    });
    return best.value();
}

void HumanPolicy::setNextAction(const Action action) { pending_ = action; }

Action HumanPolicy::selectAction(const Observation&, const std::span<const Action> legalActions) {
    requireActions(legalActions);
    if (!pending_.has_value()) {
        throw std::invalid_argument("HumanPolicy: la interfaz no entregó ninguna acción");
    }
    const Action action = *pending_;
    pending_.reset();
    return action;
}
