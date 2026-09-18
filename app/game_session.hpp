#pragma once
#include <span>
#include <vector>

#include "console_ui.hpp"

// Qué hizo la sesión con el comando recibido
enum class SessionOutcome { stepped, help, quit, finished };

// Coordina la partida: recibe un comando de la interfaz y llama a step.
// No dibuja ni lee el teclado.
class GameSession {
public:
    explicit GameSession(DemoEnvironment environment);

    SessionOutcome apply(const UiCommand& command);

    [[nodiscard]] const DemoEnvironment& environment() const noexcept;
    [[nodiscard]] std::span<const NavigationEvent> recentEvents() const noexcept;

private:
    DemoEnvironment environment_;
    std::vector<NavigationEvent> recentEvents_;
};
