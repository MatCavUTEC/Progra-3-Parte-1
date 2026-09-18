//
// Created by LucasMCgamer on 13/09/2026.
//

// Único archivo que abre una pantalla: el resto de la interfaz solo construye
// elementos y traduce eventos, así que puede probarse sin terminal.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "circuit_escape/controllers.hpp"
#include "circuit_escape/simulation.hpp"
#include "console_ui.hpp"
#include "game_session.hpp"

namespace {
    enum class AutoPolicy { none, random, heuristic };

    struct Options {
        Difficulty difficulty{Difficulty::standard};
        RenderMode mode{RenderMode::emoji};
        int map{1};
        AutoPolicy autoPolicy{AutoPolicy::none};
        std::uint32_t seed{0};
        bool help{false};
    };

    void printUsage() {
        std::cout << "Uso: navigation_game [--difficulty easy|standard|hard] [--ascii] [--map 1|2]\n"
                  << "                     [--auto random|heuristic] [--seed N] [--help]\n";
    }

    // Devuelve false si los argumentos no son válidos
    bool parseOptions(const std::vector<std::string>& arguments, Options& options) {
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            const std::string& argument = arguments[index];
            if (argument == "--ascii") {
                options.mode = RenderMode::ascii;
            } else if (argument == "--difficulty" && index + 1 < arguments.size()) {
                const std::optional<Difficulty> difficulty = parseDifficulty(arguments[++index]);
                if (!difficulty.has_value()) {
                    std::cout << "Dificultad desconocida: " << arguments[index] << "\n";
                    return false;
                }
                options.difficulty = *difficulty;
            } else if (argument == "--map" && index + 1 < arguments.size()) {
                const std::string& value = arguments[++index];
                if (value != "1" && value != "2") {
                    std::cout << "Mapa desconocido: " << value << "\n";
                    return false;
                }
                options.map = value == "1" ? 1 : 2;
            } else if (argument == "--auto" && index + 1 < arguments.size()) {
                const std::string& value = arguments[++index];
                if (value == "random") {
                    options.autoPolicy = AutoPolicy::random;
                } else if (value == "heuristic") {
                    options.autoPolicy = AutoPolicy::heuristic;
                } else {
                    std::cout << "Controlador desconocido: " << value << "\n";
                    return false;
                }
            } else if (argument == "--seed" && index + 1 < arguments.size()) {
                const std::string& value = arguments[++index];
                try {
                    options.seed = static_cast<std::uint32_t>(std::stoul(value));
                } catch (const std::exception&) {
                    std::cout << "Semilla no válida: " << value << "\n";
                    return false;
                }
            } else if (argument == "--help") {
                options.help = true;
            } else {
                return false;
            }
        }
        return true;
    }

    std::vector<std::string> readMapFile(const std::string& path) {
        std::ifstream file{path};
        if (!file) {
            throw std::runtime_error("no se pudo abrir el mapa " + path);
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    std::string reasonText(const EndReason reason) {
        switch (reason) {
            case EndReason::goalReached: return "salida alcanzada";
            case EndReason::noEnergy: return "sin energia";
            case EndReason::turnLimit: return "limite de turnos";
            case EndReason::none: return "partida sin terminar";
        }
        return "desconocido";
    }

    void printSummary(const SimulationSummary& summary) {
        std::cout << "Partida completada: " << (summary.completed ? "si" : "no") << "\n"
                  << "Motivo: " << reasonText(summary.reason) << "\n"
                  << "Turnos: " << summary.turns << "\n"
                  << "Energia restante: " << summary.energy << "/" << summary.maximumEnergy << "\n"
                  << "Recursos: " << summary.collectedResources << "\n"
                  << "Puntaje: " << summary.score << "\n";
    }

    std::unique_ptr<IController> makeController(const AutoPolicy policy, const std::uint32_t seed) {
        if (policy == AutoPolicy::random) {
            return std::make_unique<PolicyController<RandomPolicy>>(RandomPolicy{seed});
        }
        return std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{});
    }
}

int main(int argc, char* argv[]) {
    const std::vector<std::string> arguments(argv + 1, argv + argc);
    Options options;
    if (!parseOptions(arguments, options)) {
        printUsage();
        return 1;
    }
    if (options.help) {
        printUsage();
        return 0;
    }

    const GameRules rules = rulesFor(options.difficulty);
    const std::string path = std::string{CIRCUIT_ESCAPE_MAPS_DIR} +
                             (options.map == 1 ? "/scenario_01.txt" : "/scenario_02.txt");

    try {
        const Scenario<20, 30> scenario = parseScenario<20, 30>(readMapFile(path), rules);

        // Simulación automática: juega sola, sin abrir pantalla (§7, §10.2)
        if (options.autoPolicy != AutoPolicy::none) {
            DemoEnvironment environment{scenario.grid, scenario.start, rules};
            environment.reset(options.seed);
            const std::unique_ptr<IController> controller = makeController(options.autoPolicy, options.seed);
            printSummary(runSimulation(environment, *controller));
            std::cout << "Controlador: " << (options.autoPolicy == AutoPolicy::random ? "random" : "heuristic")
                      << " | Semilla: " << options.seed << "\n";
            return 0;
        }

        GameSession session{DemoEnvironment{scenario.grid, scenario.start, rules}};
        const ConsoleUI ui{options.mode};

        auto screen = ftxui::ScreenInteractive::TerminalOutput();
        std::string message;
        bool showHelp = false;

        auto component = ftxui::Renderer([&] {
            ftxui::Elements parts;
            parts.push_back(ui.render(session.environment(), session.recentEvents()));
            if (showHelp) {
                parts.push_back(ui.help());
            }
            if (!message.empty()) {
                parts.push_back(ftxui::text(message));
            }
            return ftxui::vbox(std::move(parts));
        });

        component = ftxui::CatchEvent(component, [&](const ftxui::Event& event) {
            const std::optional<UiCommand> command = ui.translate(event);
            if (!command.has_value()) {
                // Entrada desconocida: se avisa y no se ejecuta step (§5.8)
                message = "Comando desconocido. Usa WASD o flechas, E, H o Q.";
                return false;
            }
            message.clear();
            switch (session.apply(*command)) {
                case SessionOutcome::quit:
                    screen.Exit();
                    break;
                case SessionOutcome::help:
                    showHelp = !showHelp;
                    break;
                case SessionOutcome::finished:
                    message = "La partida ya terminó. Q para salir.";
                    break;
                case SessionOutcome::stepped:
                    break;
            }
            return true;
        });

        screen.Loop(component);
        printSummary(summaryOf(session.environment()));
        return 0;
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << "\n";
        return 1;
    }
}
