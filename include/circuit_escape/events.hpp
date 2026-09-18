#pragma once
#include <cstddef>
#include <variant>
#include <vector>

#include "circuit_escape/position.hpp"

// Copia inmutable de lo que un controlador puede observar (§5.7).
struct Observation {
    Position agent;
    Position goal;
    int energy{};
    int maximumEnergy{};
    int score{};
    std::size_t collectedResources{};
    std::size_t turn{};
    std::vector<Action> availableActions;
};

// Cada evento describe algo que ya ocurrió durante el turno.
struct MovedEvent { Position from; Position to; int energyCost; };
struct MovementRejectedEvent { Position from; Action action; };
struct ResourceCollectedEvent { Position at; int points; };
struct EnergyChangedEvent { int previous; int current; };
struct TrapTriggeredEvent { Position at; };
struct GoalReachedEvent { Position at; };

using NavigationEvent = std::variant<
    MovedEvent,
    MovementRejectedEvent,
    ResourceCollectedEvent,
    EnergyChangedEvent,
    TrapTriggeredEvent,
    GoalReachedEvent>;

enum class EndReason { none, goalReached, noEnergy, turnLimit };

struct StepResult {
    Observation observation;
    std::vector<NavigationEvent> events;
    bool finished{false};
    EndReason reason{EndReason::none};
};
