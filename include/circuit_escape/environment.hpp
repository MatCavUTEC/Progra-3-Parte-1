#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "circuit_escape/algorithms.hpp"
#include "circuit_escape/cells.hpp"
#include "circuit_escape/events.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/grid.hpp"
#include "circuit_escape/position.hpp"

// Única pieza que modifica el estado de la partida. No lee ni imprime (§5.7).
template<std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
public:
    using grid_type = Grid<Cell, Rows, Columns>;

    NavigationEnvironment(grid_type initialGrid, const Position start, const GameRules& rules)
        : initialGrid_(std::move(initialGrid)), rules_(rules), start_(start) {
        if (rules_.initialEnergy <= 0) {
            throw std::invalid_argument("NavigationEnvironment: la energía inicial debe ser positiva");
        }
        if (rules_.turnLimit == 0) {
            throw std::invalid_argument("NavigationEnvironment: el límite de turnos debe ser positivo");
        }
        if (!initialGrid_.contains(start_)) {
            throw std::invalid_argument("NavigationEnvironment: el inicio no pertenece al tablero");
        }
        if (!isTraversable(initialGrid_.at(start_))) {
            throw std::invalid_argument("NavigationEnvironment: el inicio no es transitable");
        }

        const std::size_t exits = countMatching(initialGrid_.begin(), initialGrid_.end(), isExit);
        if (exits != 1) {
            throw std::invalid_argument("NavigationEnvironment: debe existir exactamente una salida");
        }
        goal_ = findPosition(initialGrid_, isExit).value();

        reset(0);
    }

    // Devuelve la partida a su estado inicial. El entorno no toma decisiones
    // aleatorias: guarda la semilla para que la simulación sea reproducible.
    void reset(const std::uint32_t seed) {
        seed_ = seed;
        grid_ = initialGrid_;
        agent_ = start_;
        energy_ = rules_.initialEnergy;
        score_ = 0;
        collectedResources_ = 0;
        turn_ = 0;
        finished_ = false;
        reason_ = EndReason::none;
    }

    [[nodiscard]] Observation state() const {
        return Observation{.agent = agent_,
                           .goal = goal_,
                           .energy = energy_,
                           .maximumEnergy = rules_.initialEnergy,
                           .score = score_,
                           .collectedResources = collectedResources_,
                           .turn = turn_,
                           .availableActions = availableActions()};
    }

    // Movimientos que producirían un desplazamiento válido, además de wait.
    // Una partida terminada no ofrece ninguna acción.
    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;
        if (finished_) {
            return actions;
        }
        actions.push_back(Action::wait);
        for (const Action action : {Action::up, Action::down, Action::left, Action::right}) {
            if (targetOf(action).has_value()) {
                actions.push_back(action);
            }
        }
        return actions;
    }

    [[nodiscard]] bool isFinished() const noexcept { return finished_; }
    [[nodiscard]] const GameRules& rules() const noexcept { return rules_; }
    [[nodiscard]] EndReason endReason() const noexcept { return reason_; }
    [[nodiscard]] const grid_type& grid() const noexcept { return grid_; }
    [[nodiscard]] std::uint32_t seed() const noexcept { return seed_; }

    // Resuelve un turno en el orden obligatorio de §5.5.
    [[nodiscard]] StepResult step(const Action action) {
        if (finished_) {
            throw std::logic_error("NavigationEnvironment: la partida ya terminó");
        }

        std::vector<NavigationEvent> events;
        ++turn_;

        if (action == Action::wait) {
            // No activa la celda donde está el agente
            spendEnergy(rules_.waitCost, events);
        } else if (const std::optional<Position> target = targetOf(action); target.has_value()) {
            const Position from = agent_;
            agent_ = *target;
            const int entryCost = entryCostOf(grid_.at(agent_));
            appendEvents(events, MovedEvent{from, agent_, entryCost});
            spendEnergy(entryCost, events);
            applyCellEffect(events);
        } else {
            appendEvents(events, MovementRejectedEvent{agent_, action});
            spendEnergy(rules_.rejectedMoveCost, events);
        }

        checkEnd(events);
        return StepResult{.observation = state(),
                          .events = std::move(events),
                          .finished = finished_,
                          .reason = reason_};
    }

private:
    static bool isExit(const Cell& cell) { return std::holds_alternative<Exit>(cell); }

    // Posición a la que llevaría la acción, si el desplazamiento es válido.
    [[nodiscard]] std::optional<Position> targetOf(const Action action) const {
        const std::optional<Position> candidate = neighbor(agent_, action);
        if (!candidate.has_value() || !grid_.contains(*candidate) || !isTraversable(grid_.at(*candidate))) {
            return std::nullopt;
        }
        return candidate;
    }

    // El terreno elevado cobra su propio costo; el resto de celdas, el costo normal.
    [[nodiscard]] int entryCostOf(const Cell& cell) const {
        return std::visit(Overloaded{
                              [](const RoughTerrain& rough) { return rough.energyCost; },
                              [this](const auto&) { return rules_.moveCost; },
                          },
                          cell);
    }

    // Efecto de la celda a la que acaba de entrar el agente. Se aplica aunque el
    // costo de entrada haya dejado la energía en cero, así que una batería todavía
    // puede recargar. Cada tipo de celda tiene su lambda, incluidos los que no
    // hacen nada, para que un tipo nuevo no pase inadvertido.
    void applyCellEffect(std::vector<NavigationEvent>& events) {
        std::visit(Overloaded{
                       [](Empty&) {},
                       [](Wall&) {},          // el agente nunca entra a un muro
                       [](RoughTerrain&) {},  // su costo ya se cobró al entrar
                       [this, &events](ResourceCell<int>& resource) {
                           if (resource.collected) {
                               return;
                           }
                           resource.collected = true;
                           score_ += resource.reward;
                           ++collectedResources_;
                           appendEvents(events, ResourceCollectedEvent{agent_, resource.reward});
                       },
                       [this, &events](Battery& battery) {
                           if (battery.consumed) {
                               return;
                           }
                           battery.consumed = true;
                           changeEnergy(battery.energy, events);
                       },
                       [this, &events](Trap& trap) {
                           appendEvents(events, TrapTriggeredEvent{agent_});
                           changeEnergy(-trap.energyPenalty, events);
                           score_ -= trap.scorePenalty;  // el puntaje puede quedar negativo
                       },
                       [](Exit&) {},  // la victoria se decide al comprobar el término
                   },
                   grid_.at(agent_));
    }

    void spendEnergy(const int cost, std::vector<NavigationEvent>& events) { changeEnergy(-cost, events); }

    // Toda modificación de energía queda entre cero y la energía máxima.
    void changeEnergy(const int delta, std::vector<NavigationEvent>& events) {
        const int previous = energy_;
        energy_ = clampEnergy(energy_ + delta);
        if (energy_ != previous) {
            appendEvents(events, EnergyChangedEvent{previous, energy_});
        }
    }

    [[nodiscard]] int clampEnergy(const int value) const {
        if (value < 0) return 0;
        if (value > rules_.initialEnergy) return rules_.initialEnergy;
        return value;
    }

    // Precedencia obligatoria: llegada a la salida, energía agotada, límite de turnos.
    void checkEnd(std::vector<NavigationEvent>& events) {
        if (agent_ == goal_ && energy_ >= 1) {
            finished_ = true;
            reason_ = EndReason::goalReached;
            appendEvents(events, GoalReachedEvent{agent_});
            return;
        }
        if (energy_ == 0) {
            finished_ = true;
            reason_ = EndReason::noEnergy;
            return;
        }
        if (turn_ >= rules_.turnLimit) {
            finished_ = true;
            reason_ = EndReason::turnLimit;
        }
    }

    grid_type initialGrid_;
    GameRules rules_;
    Position start_;
    Position goal_;

    grid_type grid_;
    Position agent_;
    int energy_{};
    int score_{};
    std::size_t collectedResources_{};
    std::size_t turn_{};
    bool finished_{false};
    EndReason reason_{EndReason::none};
    std::uint32_t seed_{};
};
