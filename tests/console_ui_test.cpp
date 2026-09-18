// Pruebas de la interfaz de consola (§5.8, §8). Construyen elementos y los dibujan
// en una pantalla en memoria: no abren una interfaz interactiva.

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <string>
#include <variant>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "console_ui.hpp"
#include "game_session.hpp"

namespace {
    DemoEnvironment makeEnvironment() {
        const GameRules rules = rulesFor(Difficulty::standard);
        std::vector<std::string> lines;
        lines.push_back(std::string(30, '#'));
        lines.push_back("#@..R.B.T.~.................." "#");
        for (std::size_t row = 2; row < 18; ++row) {
            lines.push_back("#" + std::string(28, '.') + "#");
        }
        lines.push_back("#" + std::string(27, '.') + "S#");
        lines.push_back(std::string(30, '#'));
        const Scenario<20, 30> scenario = parseScenario<20, 30>(lines, rules);
        return DemoEnvironment{scenario.grid, scenario.start, rules};
    }

    std::string renderToText(const ftxui::Element& element, const int width, const int height) {
        ftxui::Screen screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                                     ftxui::Dimension::Fixed(height));
        ftxui::Render(screen, element);
        return screen.ToString();
    }

    std::vector<std::string> splitLines(const std::string& text) {
        std::vector<std::string> lines;
        std::string current;
        for (const char symbol : text) {
            if (symbol == '\n') {
                lines.push_back(current);
                current.clear();
            } else if (symbol != '\r') {  // FTXUI termina cada línea con \r\n
                current.push_back(symbol);
            }
        }
        if (!current.empty()) {
            lines.push_back(current);
        }
        return lines;
    }

    bool holdsAction(const std::optional<UiCommand>& command, const Action action) {
        return command.has_value() && std::holds_alternative<Action>(*command) &&
               std::get<Action>(*command) == action;
    }

    void testTranslateMovement() {
        const ConsoleUI ui;
        assert(holdsAction(ui.translate(ftxui::Event::Character('w')), Action::up));
        assert(holdsAction(ui.translate(ftxui::Event::Character('a')), Action::left));
        assert(holdsAction(ui.translate(ftxui::Event::Character('s')), Action::down));
        assert(holdsAction(ui.translate(ftxui::Event::Character('d')), Action::right));
        assert(holdsAction(ui.translate(ftxui::Event::Character('e')), Action::wait));
    }

    void testTranslateIgnoresCase() {
        const ConsoleUI ui;
        assert(holdsAction(ui.translate(ftxui::Event::Character('W')), Action::up));
        assert(holdsAction(ui.translate(ftxui::Event::Character('A')), Action::left));
        assert(holdsAction(ui.translate(ftxui::Event::Character('S')), Action::down));
        assert(holdsAction(ui.translate(ftxui::Event::Character('D')), Action::right));
        assert(holdsAction(ui.translate(ftxui::Event::Character('E')), Action::wait));
    }

    void testTranslateArrows() {
        const ConsoleUI ui;
        assert(holdsAction(ui.translate(ftxui::Event::ArrowUp), Action::up));
        assert(holdsAction(ui.translate(ftxui::Event::ArrowDown), Action::down));
        assert(holdsAction(ui.translate(ftxui::Event::ArrowLeft), Action::left));
        assert(holdsAction(ui.translate(ftxui::Event::ArrowRight), Action::right));
    }

    void testTranslateHelpAndQuit() {
        const ConsoleUI ui;
        const std::optional<UiCommand> help = ui.translate(ftxui::Event::Character('H'));
        const std::optional<UiCommand> quit = ui.translate(ftxui::Event::Character('q'));
        assert(help.has_value() && std::holds_alternative<HelpCommand>(*help));
        assert(quit.has_value() && std::holds_alternative<QuitCommand>(*quit));
    }

    void testTranslateRejectsUnknownKeys() {
        const ConsoleUI ui;
        assert(!ui.translate(ftxui::Event::Character('z')).has_value());
        assert(!ui.translate(ftxui::Event::Character('1')).has_value());
        assert(!ui.translate(ftxui::Event::Return).has_value());
        assert(!ui.translate(ftxui::Event::Escape).has_value());
    }

    // §8: un comando desconocido no modifica el entorno
    void testUnknownKeyLeavesTheEnvironmentUntouched() {
        const ConsoleUI ui;
        GameSession session{makeEnvironment()};
        const std::optional<UiCommand> command = ui.translate(ftxui::Event::Character('z'));
        assert(!command.has_value());
        assert(session.environment().state().turn == 0);

        const std::optional<UiCommand> move = ui.translate(ftxui::Event::Character('d'));
        assert(move.has_value());
        assert(session.apply(*move) == SessionOutcome::stepped);
        assert(session.environment().state().turn == 1);
    }

    void testSessionHandlesHelpAndQuit() {
        GameSession session{makeEnvironment()};
        assert(session.apply(UiCommand{HelpCommand{}}) == SessionOutcome::help);
        assert(session.apply(UiCommand{QuitCommand{}}) == SessionOutcome::quit);
        assert(session.environment().state().turn == 0);
        assert(session.recentEvents().empty());
    }

    void testSessionKeepsRecentEvents() {
        GameSession session{makeEnvironment()};
        assert(session.apply(UiCommand{Action::right}) == SessionOutcome::stepped);
        assert(!session.recentEvents().empty());
        assert(std::holds_alternative<MovedEvent>(session.recentEvents().front()));
    }

    // §8: representación emoji y ASCII de todas las clases de celda
    void testGlyphsForEveryCell() {
        const ConsoleUI emoji{RenderMode::emoji};
        const ConsoleUI ascii{RenderMode::ascii};

        assert(ascii.glyphFor(Cell{Empty{}}) == ".");
        assert(ascii.glyphFor(Cell{Wall{}}) == "#");
        assert(ascii.glyphFor(Cell{RoughTerrain{}}) == "~");
        assert(ascii.glyphFor(Cell{ResourceCell<int>{10}}) == "R");
        assert(ascii.glyphFor(Cell{Battery{}}) == "B");
        assert(ascii.glyphFor(Cell{Trap{}}) == "T");
        assert(ascii.glyphFor(Cell{Exit{}}) == "S");
        assert(ascii.agentGlyph() == "@");

        assert(emoji.glyphFor(Cell{Empty{}}) == "⬜");
        assert(emoji.glyphFor(Cell{Wall{}}) == "⬛");
        assert(emoji.glyphFor(Cell{RoughTerrain{}}) == "🟫");
        assert(emoji.glyphFor(Cell{ResourceCell<int>{10}}) == "💎");
        assert(emoji.glyphFor(Cell{Battery{}}) == "⚡");
        assert(emoji.glyphFor(Cell{Trap{}}) == "💥");
        assert(emoji.glyphFor(Cell{Exit{}}) == "🏁");
        assert(emoji.agentGlyph() == "🤖");
    }

    void testConsumedCellsLookEmpty() {
        const ConsoleUI ascii{RenderMode::ascii};
        const ConsoleUI emoji{RenderMode::emoji};
        const ResourceCell<int> collected{10, true};
        const Battery consumed{3, true};
        assert(ascii.glyphFor(Cell{collected}) == ".");
        assert(ascii.glyphFor(Cell{consumed}) == ".");
        assert(emoji.glyphFor(Cell{collected}) == "⬜");
        assert(emoji.glyphFor(Cell{consumed}) == "⬜");
    }

    // §8: el renderizado conserva 20 filas y 30 celdas por fila
    void testBoardKeepsItsShape() {
        const ConsoleUI ui{RenderMode::ascii};
        const DemoEnvironment environment = makeEnvironment();
        const std::vector<std::vector<std::string>> board = ui.boardGlyphs(environment);
        assert(board.size() == 20);
        for (const std::vector<std::string>& row : board) {
            assert(row.size() == 30);
        }

        const Position start{1, 1};
        assert(board[start.row][start.column] == ui.agentGlyph());
        assert(board[0][0] == ui.glyphFor(Cell{Wall{}}));
        assert(board[18][28] == ui.glyphFor(Cell{Exit{}}));
        assert(board[19][0] == ui.glyphFor(Cell{Wall{}}));
    }

    void testStatusLine() {
        const ConsoleUI ui;
        Observation observation;
        observation.turn = 27;
        observation.energy = 42;
        observation.maximumEnergy = 60;
        observation.score = 20;
        observation.collectedResources = 2;
        const std::string line = ui.statusLine(observation, 180, 3);
        assert(line == "Turno 27/180 | Energía 42/60 | Puntaje 20 | Recursos 2/3");
    }

    void testDescribeEveryEvent() {
        const ConsoleUI ui;
        const Position at{10, 24};
        assert(!ui.describeEvent(MovedEvent{at, at, 1}).empty());
        assert(!ui.describeEvent(MovementRejectedEvent{at, Action::up}).empty());
        assert(ui.describeEvent(ResourceCollectedEvent{at, 10}).find("+10") != std::string::npos);
        assert(ui.describeEvent(ResourceCollectedEvent{at, 10}).find("(10,24)") != std::string::npos);
        assert(!ui.describeEvent(EnergyChangedEvent{5, 4}).empty());
        assert(!ui.describeEvent(TrapTriggeredEvent{at}).empty());
        assert(!ui.describeEvent(GoalReachedEvent{at}).empty());
    }

    // El árbol se dibuja en una pantalla en memoria: sin terminal ni ScreenInteractive
    void testRenderOffScreen() {
        const ConsoleUI ui{RenderMode::ascii};
        const DemoEnvironment environment = makeEnvironment();
        const std::vector<NavigationEvent> events{ResourceCollectedEvent{Position{1, 4}, 10}};
        // Una línea de estado, una de coordenadas, veinte de tablero y el pie
        const std::string text = renderToText(ui.render(environment, events), 62, 23);
        const std::vector<std::string> lines = splitLines(text);

        assert(lines.size() == 23);
        assert(lines.front().find("Turno 0/180") != std::string::npos);
        assert(lines.back().find("WASD") != std::string::npos);

        // Una fila de coordenadas más veinte filas de tablero, todas de 62 columnas
        for (std::size_t row = 1; row <= 21; ++row) {
            assert(lines[row].size() == 62);
        }
        // Estado, coordenadas, fila 0 del tablero y fila 1, donde está el agente
        assert(lines[3].find(ui.agentGlyph()) != std::string::npos);
        assert(lines[2].find(ui.agentGlyph()) == std::string::npos);
    }

    void testHelpRendersOffScreen() {
        const ConsoleUI ui{RenderMode::ascii};
        const std::string text = renderToText(ui.help(), 62, 10);
        assert(text.find("WASD") != std::string::npos);
        assert(text.find("Q") != std::string::npos);
    }
}

int main() {
    testTranslateMovement();
    testTranslateIgnoresCase();
    testTranslateArrows();
    testTranslateHelpAndQuit();
    testTranslateRejectsUnknownKeys();
    testUnknownKeyLeavesTheEnvironmentUntouched();
    testSessionHandlesHelpAndQuit();
    testSessionKeepsRecentEvents();
    testGlyphsForEveryCell();
    testConsumedCellsLookEmpty();
    testBoardKeepsItsShape();
    testStatusLine();
    testDescribeEveryEvent();
    testRenderOffScreen();
    testHelpRendersOffScreen();
    return 0;
}
