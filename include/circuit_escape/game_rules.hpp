#pragma once
#include <cstddef>
#include <optional>
#include <string_view>

enum class Difficulty { easy, standard, hard };

// Única fuente de los valores configurables (§5.5). Los valores por defecto son
// los del perfil standard, igual que los de las celdas.
// Las penalizaciones de la trampa se guardan como cantidades a restar.
struct GameRules {
    int initialEnergy{60};
    std::size_t turnLimit{180};
    int moveCost{1};
    int roughTerrainCost{2};
    int waitCost{1};
    int rejectedMoveCost{1};
    int resourcePoints{10};
    int batteryEnergy{3};
    int trapEnergyPenalty{2};
    int trapScorePenalty{1};

    friend bool operator==(const GameRules&, const GameRules&) = default;
};

[[nodiscard]] GameRules rulesFor(Difficulty difficulty);

// Lee la opción --difficulty; devuelve nullopt si el texto no es un perfil.
[[nodiscard]] std::optional<Difficulty> parseDifficulty(std::string_view text);
