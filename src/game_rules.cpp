#include "circuit_escape/game_rules.hpp"

#include <stdexcept>

GameRules rulesFor(const Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::easy:
            return GameRules{.initialEnergy = 80, .turnLimit = 240,
                             .moveCost = 1, .roughTerrainCost = 2,
                             .waitCost = 1, .rejectedMoveCost = 1,
                             .resourcePoints = 15, .batteryEnergy = 5,
                             .trapEnergyPenalty = 1, .trapScorePenalty = 0};
        case Difficulty::standard:
            return GameRules{.initialEnergy = 60, .turnLimit = 180,
                             .moveCost = 1, .roughTerrainCost = 2,
                             .waitCost = 1, .rejectedMoveCost = 1,
                             .resourcePoints = 10, .batteryEnergy = 3,
                             .trapEnergyPenalty = 2, .trapScorePenalty = 1};
        case Difficulty::hard:
            return GameRules{.initialEnergy = 40, .turnLimit = 140,
                             .moveCost = 1, .roughTerrainCost = 3,
                             .waitCost = 1, .rejectedMoveCost = 1,
                             .resourcePoints = 8, .batteryEnergy = 2,
                             .trapEnergyPenalty = 3, .trapScorePenalty = 2};
    }
    throw std::invalid_argument("rulesFor: dificultad no válida");
}

std::optional<Difficulty> parseDifficulty(const std::string_view text) {
    if (text == "easy") return Difficulty::easy;
    if (text == "standard") return Difficulty::standard;
    if (text == "hard") return Difficulty::hard;
    return std::nullopt;
}
