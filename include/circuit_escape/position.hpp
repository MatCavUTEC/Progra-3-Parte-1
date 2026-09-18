#pragma once
#include <cstddef>
#include <optional>
#include <string>

struct Position {
    std::size_t row{};
    std::size_t column{};

    friend bool operator==(const Position&, const Position&) = default;
};

enum class Action { up, down, left, right, wait };

// No conoce el tamaño del tablero: solo descarta filas o columnas negativas.
// El límite superior lo valida Grid.
[[nodiscard]] std::optional<Position> neighbor(Position origin, Action action);

[[nodiscard]] std::string toString(Position position);

// Distancia en pasos horizontales y verticales, sin diagonales.
[[nodiscard]] std::size_t manhattanDistance(Position from, Position to);
