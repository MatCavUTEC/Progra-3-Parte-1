#pragma once
#include <concepts>
#include <cstdint>
#include <optional>
#include <random>
#include <span>
#include <utility>

#include "circuit_escape/events.hpp"
#include "circuit_escape/position.hpp"

// Interfaz común de los controladores automáticos (§5.7).
class IController {
public:
    virtual ~IController() = default;
    virtual Action selectAction(const Observation& observation, std::span<const Action> legalActions) = 0;
};

// Contrato que debe cumplir una política para poder adaptarse a IController.
template<typename Policy>
concept NavigationPolicy = requires(Policy& policy,
                                    const Observation& observation,
                                    std::span<const Action> actions) {
    { policy.selectAction(observation, actions) } -> std::same_as<Action>;
};

// Adaptador genérico: el concept verifica el contrato al compilar y la interfaz
// virtual permite cambiar de controlador en ejecución, sin preguntar por el tipo.
template<NavigationPolicy Policy>
class PolicyController final : public IController {
public:
    explicit PolicyController(Policy policy) : policy_(std::move(policy)) {}

    Action selectAction(const Observation& observation, std::span<const Action> legalActions) override {
        return policy_.selectAction(observation, legalActions);
    }

private:
    Policy policy_;
};

// Elige una acción legal al azar. La semilla hace reproducible la simulación (§9).
class RandomPolicy {
public:
    explicit RandomPolicy(std::uint32_t seed);

    Action selectAction(const Observation& observation, std::span<const Action> legalActions);

private:
    std::mt19937 generator_;
};

// Se acerca a la salida reduciendo la distancia Manhattan. No busca el camino
// óptimo: solo mira las acciones legales de este turno (§5.6).
class HeuristicPolicy {
public:
    Action selectAction(const Observation& observation, std::span<const Action> legalActions);
};

// Entrega la decisión que le pasó la interfaz de consola.
class HumanPolicy {
public:
    void setNextAction(Action action);

    Action selectAction(const Observation& observation, std::span<const Action> legalActions);

private:
    std::optional<Action> pending_;
};
