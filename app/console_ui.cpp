#include "console_ui.hpp"

#include <array>
#include <cctype>

#include "circuit_escape/algorithms.hpp"

// Sin "using namespace ftxui": la biblioteca también define un tipo Cell y
// nuestras celdas dejarían de resolverse.
namespace {
    // Cada celda y cada coordenada ocupan exactamente dos columnas, para que el
    // emoji y el número compartan la misma geometría (§5.8).
    ftxui::Element fixedCell(std::string label) {
        return ftxui::text(std::move(label)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 2);
    }

    const std::array<std::string, 10> keycapDigits{
        "0️⃣", "1️⃣", "2️⃣", "3️⃣", "4️⃣",
        "5️⃣", "6️⃣", "7️⃣", "8️⃣", "9️⃣"};

    bool isCollectibleResource(const Cell& cell) { return isCollectible(cell); }
}

ConsoleUI::ConsoleUI(const RenderMode mode) : mode_(mode) {}

RenderMode ConsoleUI::mode() const noexcept { return mode_; }

std::optional<UiCommand> ConsoleUI::translate(const ftxui::Event& event) const {
    if (event == ftxui::Event::ArrowUp) return UiCommand{Action::up};
    if (event == ftxui::Event::ArrowDown) return UiCommand{Action::down};
    if (event == ftxui::Event::ArrowLeft) return UiCommand{Action::left};
    if (event == ftxui::Event::ArrowRight) return UiCommand{Action::right};

    if (!event.is_character() || event.input().size() != 1) {
        return std::nullopt;
    }
    switch (std::tolower(static_cast<unsigned char>(event.input().front()))) {
        case 'w': return UiCommand{Action::up};
        case 's': return UiCommand{Action::down};
        case 'a': return UiCommand{Action::left};
        case 'd': return UiCommand{Action::right};
        case 'e': return UiCommand{Action::wait};
        case 'h': return UiCommand{HelpCommand{}};
        case 'q': return UiCommand{QuitCommand{}};
        default: return std::nullopt;
    }
}

std::string ConsoleUI::agentGlyph() const { return mode_ == RenderMode::emoji ? "🤖" : "@"; }

std::string ConsoleUI::glyphFor(const Cell& cell) const {
    const bool emoji = mode_ == RenderMode::emoji;
    const std::string empty = emoji ? "⬜" : ".";
    return std::visit(Overloaded{
                          [&](const Empty&) { return empty; },
                          [&](const Wall&) { return emoji ? std::string{"⬛"} : std::string{"#"}; },
                          [&](const RoughTerrain&) { return emoji ? std::string{"🟫"} : std::string{"~"}; },
                          // Una celda ya usada se ve como espacio libre
                          [&](const ResourceCell<int>& resource) {
                              if (resource.collected) return empty;
                              return emoji ? std::string{"💎"} : std::string{"R"};
                          },
                          [&](const Battery& battery) {
                              if (battery.consumed) return empty;
                              return emoji ? std::string{"⚡"} : std::string{"B"};
                          },
                          [&](const Trap&) { return emoji ? std::string{"💥"} : std::string{"T"}; },
                          [&](const Exit&) { return emoji ? std::string{"🏁"} : std::string{"S"}; },
                      },
                      cell);
}

std::vector<std::vector<std::string>> ConsoleUI::boardGlyphs(const DemoEnvironment& environment) const {
    const Position agent = environment.state().agent;
    std::vector<std::vector<std::string>> board;
    board.reserve(DemoEnvironment::grid_type::rows());
    for (std::size_t row = 0; row < DemoEnvironment::grid_type::rows(); ++row) {
        std::vector<std::string> glyphs;
        glyphs.reserve(DemoEnvironment::grid_type::columns());
        for (std::size_t column = 0; column < DemoEnvironment::grid_type::columns(); ++column) {
            const Position position{row, column};
            glyphs.push_back(position == agent ? agentGlyph() : glyphFor(environment.grid().at(position)));
        }
        board.push_back(std::move(glyphs));
    }
    return board;
}

std::string ConsoleUI::statusLine(const Observation& observation,
                                  const std::size_t turnLimit,
                                  const std::size_t totalResources) const {
    return "Turno " + std::to_string(observation.turn) + "/" + std::to_string(turnLimit) +
           " | Energía " + std::to_string(observation.energy) + "/" + std::to_string(observation.maximumEnergy) +
           " | Puntaje " + std::to_string(observation.score) +
           " | Recursos " + std::to_string(observation.collectedResources) + "/" + std::to_string(totalResources);
}

std::string ConsoleUI::describeEvent(const NavigationEvent& event) const {
    return std::visit(Overloaded{
                          [](const MovedEvent& moved) { return "Avance a " + toString(moved.to); },
                          [](const MovementRejectedEvent& rejected) {
                              return "Movimiento bloqueado en " + toString(rejected.from);
                          },
                          [](const ResourceCollectedEvent& collected) {
                              return "+" + std::to_string(collected.points) + " en " + toString(collected.at);
                          },
                          [](const EnergyChangedEvent& energy) {
                              return "Energía " + std::to_string(energy.current);
                          },
                          [](const TrapTriggeredEvent& trap) { return "Trampa en " + toString(trap.at); },
                          [](const GoalReachedEvent& goal) { return "Salida alcanzada en " + toString(goal.at); },
                      },
                      event);
}

std::string ConsoleUI::coordinateLabel(const std::size_t index) const {
    const std::size_t digit = index % 10;
    if (mode_ == RenderMode::emoji) {
        return keycapDigits[digit];
    }
    return std::to_string(digit);
}

ftxui::Element ConsoleUI::render(const DemoEnvironment& environment,
                                 const std::span<const NavigationEvent> recentEvents) const {
    const Observation observation = environment.state();
    const std::size_t totalResources =
        countMatching(environment.grid().begin(), environment.grid().end(), isCollectibleResource);

    ftxui::Elements rows;
    rows.push_back(ftxui::text(statusLine(observation, environment.rules().turnLimit, totalResources)));

    // Regla horizontal: la esquina usa el mismo glifo que un espacio vacío
    ftxui::Elements horizontal;
    horizontal.push_back(fixedCell(glyphFor(Cell{Empty{}})));
    for (std::size_t column = 0; column < DemoEnvironment::grid_type::columns(); ++column) {
        horizontal.push_back(fixedCell(coordinateLabel(column)));
    }
    rows.push_back(ftxui::hbox(std::move(horizontal)));

    const std::vector<std::vector<std::string>> board = boardGlyphs(environment);
    for (std::size_t row = 0; row < board.size(); ++row) {
        ftxui::Elements cells;
        cells.push_back(fixedCell(coordinateLabel(row)));
        for (const std::string& glyph : board[row]) {
            cells.push_back(fixedCell(glyph));
        }
        rows.push_back(ftxui::hbox(std::move(cells)));
    }

    const std::string lastEvent = recentEvents.empty() ? std::string{"Listo"}
                                                       : describeEvent(recentEvents.back());
    rows.push_back(ftxui::text(lastEvent + " | WASD mover · E esperar · H ayuda · Q salir"));
    return ftxui::vbox(std::move(rows));
}

ftxui::Element ConsoleUI::help() const {
    return ftxui::vbox({
        ftxui::text("Controles"),
        ftxui::text("  WASD o flechas: mover"),
        ftxui::text("  E: esperar un turno"),
        ftxui::text("  H: mostrar u ocultar esta ayuda"),
        ftxui::text("  Q: salir de la partida"),
    });
}
