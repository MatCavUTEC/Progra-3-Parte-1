#pragma once
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/events.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/position.hpp"
#include "circuit_escape/scenario.hpp"

// El escenario de demostración es de 20 x 30 (§5.1)
using DemoEnvironment = NavigationEnvironment<20, 30>;

struct QuitCommand {};
struct HelpCommand {};
using UiCommand = std::variant<Action, QuitCommand, HelpCommand>;

enum class RenderMode { emoji, ascii };

// Solo presenta: no aplica energía, no mueve al agente y no modifica celdas (§5.7).
class ConsoleUI {
public:
    explicit ConsoleUI(RenderMode mode = RenderMode::emoji);

    [[nodiscard]] RenderMode mode() const noexcept;

    // Traduce una pulsación. Devuelve nullopt si la tecla no significa nada.
    [[nodiscard]] std::optional<UiCommand> translate(const ftxui::Event& event) const;

    [[nodiscard]] std::string agentGlyph() const;
    [[nodiscard]] std::string glyphFor(const Cell& cell) const;

    // Tablero completo como texto: 20 filas de 30 glifos, con el agente encima
    [[nodiscard]] std::vector<std::vector<std::string>> boardGlyphs(const DemoEnvironment& environment) const;

    [[nodiscard]] std::string statusLine(const Observation& observation,
                                         std::size_t turnLimit,
                                         std::size_t totalResources) const;
    [[nodiscard]] std::string describeEvent(const NavigationEvent& event) const;

    [[nodiscard]] ftxui::Element render(const DemoEnvironment& environment,
                                        std::span<const NavigationEvent> recentEvents) const;
    [[nodiscard]] ftxui::Element help() const;

private:
    [[nodiscard]] std::string coordinateLabel(std::size_t index) const;

    RenderMode mode_;
};
