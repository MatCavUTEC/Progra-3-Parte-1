#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/grid.hpp"
#include "circuit_escape/position.hpp"

// Un escenario es el tablero más la posición donde empieza el agente.
template<std::size_t Rows, std::size_t Columns>
struct Scenario {
    Grid<Cell, Rows, Columns> grid;
    Position start{};
};

// Símbolos del mapa, con la convención ASCII de §5.8. '@' marca el inicio del
// agente y su celda queda como espacio libre.
inline constexpr char startSymbol = '@';

[[nodiscard]] Cell cellFromSymbol(char symbol, const GameRules& rules);

// Convierte las líneas de texto de un mapa. Solo valida el formato: la cantidad
// de líneas, su longitud, los símbolos y que haya exactamente un inicio.
template<std::size_t Rows, std::size_t Columns>
[[nodiscard]] Scenario<Rows, Columns> parseScenario(const std::vector<std::string>& lines,
                                                    const GameRules& rules) {
    if (lines.size() != Rows) {
        throw std::invalid_argument("parseScenario: el mapa debe tener " + std::to_string(Rows) + " filas");
    }

    Scenario<Rows, Columns> scenario;
    std::size_t starts = 0;
    for (std::size_t row = 0; row < Rows; ++row) {
        const std::string& line = lines[row];
        if (line.size() != Columns) {
            throw std::invalid_argument("parseScenario: la fila " + std::to_string(row) +
                                        " debe tener " + std::to_string(Columns) + " columnas");
        }
        for (std::size_t column = 0; column < Columns; ++column) {
            const char symbol = line[column];
            const Position position{row, column};
            scenario.grid.at(position) = cellFromSymbol(symbol, rules);
            if (symbol == startSymbol) {
                scenario.start = position;
                ++starts;
            }
        }
    }

    if (starts != 1) {
        throw std::invalid_argument("parseScenario: el mapa debe marcar exactamente un inicio con '@'");
    }
    return scenario;
}
