#pragma once
#include <variant>

// Los valores por defecto son los del perfil standard (§5.5). Al construir un
// tablero, cada celda recibe los valores del GameRules elegido.
struct Empty {};
struct Wall {};
struct RoughTerrain { int energyCost{2}; };

template<typename Reward>
struct ResourceCell {
    Reward reward{};
    bool collected{false};
};

struct Battery { int energy{3}; bool consumed{false}; };
struct Trap { int energyPenalty{2}; int scorePenalty{1}; };
struct Exit {};

using Cell = std::variant<
    Empty, Wall, RoughTerrain, ResourceCell<int>, Battery, Trap, Exit>;

// Rasgos que dependen solo del tipo de celda. No incluyen costos ni recompensas:
// esos números vienen de GameRules y se guardan en cada celda.
template<typename CellType>
struct CellTraits {
    static constexpr bool traversable = true;
    static constexpr bool collectible = false;
};

// Especialización total: el muro es el único tipo que bloquea el paso.
template<>
struct CellTraits<Wall> {
    static constexpr bool traversable = false;
    static constexpr bool collectible = false;
};

// Especialización parcial: toda la familia de recursos cuenta como recurso del
// mapa, sin importar el tipo de su recompensa.
template<typename Reward>
struct CellTraits<ResourceCell<Reward>> {
    static constexpr bool traversable = true;
    static constexpr bool collectible = true;
};

// Consultan el rasgo del tipo que el Cell contiene en este momento.
[[nodiscard]] bool isTraversable(const Cell& cell);
[[nodiscard]] bool isCollectible(const Cell& cell);
