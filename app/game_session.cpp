#include "game_session.hpp"

#include <utility>

#include "circuit_escape/algorithms.hpp"

GameSession::GameSession(DemoEnvironment environment) : environment_(std::move(environment)) {}

SessionOutcome GameSession::apply(const UiCommand& command) {
    return std::visit(Overloaded{
                          [this](const Action action) {
                              if (environment_.isFinished()) {
                                  return SessionOutcome::finished;
                              }
                              StepResult result = environment_.step(action);
                              recentEvents_ = std::move(result.events);
                              return SessionOutcome::stepped;
                          },
                          [](const HelpCommand&) { return SessionOutcome::help; },
                          [](const QuitCommand&) { return SessionOutcome::quit; },
                      },
                      command);
}

const DemoEnvironment& GameSession::environment() const noexcept { return environment_; }

std::span<const NavigationEvent> GameSession::recentEvents() const noexcept { return recentEvents_; }
