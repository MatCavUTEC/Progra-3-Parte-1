#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>

#include "circuit_escape/position.hpp"

template<typename CellType, std::size_t Rows, std::size_t Columns>
class Grid {
    static_assert(Rows > 0 && Columns > 0, "Grid: las dimensiones deben ser positivas");

public:
    using value_type = CellType;
    using iterator = typename std::array<CellType, Rows * Columns>::iterator;
    using const_iterator = typename std::array<CellType, Rows * Columns>::const_iterator;

    static constexpr std::size_t rows() noexcept { return Rows; }
    static constexpr std::size_t columns() noexcept { return Columns; }

    [[nodiscard]] constexpr bool contains(const Position position) const noexcept {
        return position.row < Rows && position.column < Columns;
    }

    CellType& at(const Position position) { return cells_[indexOf(position)]; }
    [[nodiscard]] const CellType& at(const Position position) const { return cells_[indexOf(position)]; }

    iterator begin() noexcept { return cells_.begin(); }
    iterator end() noexcept { return cells_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return cells_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return cells_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return cells_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return cells_.cend(); }

private:
    // Las celdas se guardan por filas: primero la fila 0 completa, luego la 1, etc.
    static std::size_t indexOf(const Position position) {
        if (position.row >= Rows || position.column >= Columns) {
            throw std::out_of_range("Grid: posición fuera del tablero " + toString(position));
        }
        return position.row * Columns + position.column;
    }

    std::array<CellType, Rows * Columns> cells_{};
};
