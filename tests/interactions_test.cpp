//
// Created by LucasMCgamer on 13/09/2026.
//

// Antes de cualquier include: assert sigue activo aunque Release defina NDEBUG
#undef NDEBUG
#include <cassert>
#include <optional>

#include "circuit_escape/game_rules.hpp"

namespace {
    void testStandardProfile() {
        const GameRules rules = rulesFor(Difficulty::standard);
        assert(rules.initialEnergy == 60);
        assert(rules.turnLimit == 180);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 2);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 10);
        assert(rules.batteryEnergy == 3);
        assert(rules.trapEnergyPenalty == 2);
        assert(rules.trapScorePenalty == 1);
    }

    void testEasyProfile() {
        const GameRules rules = rulesFor(Difficulty::easy);
        assert(rules.initialEnergy == 80);
        assert(rules.turnLimit == 240);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 2);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 15);
        assert(rules.batteryEnergy == 5);
        assert(rules.trapEnergyPenalty == 1);
        assert(rules.trapScorePenalty == 0);
    }

    void testHardProfile() {
        const GameRules rules = rulesFor(Difficulty::hard);
        assert(rules.initialEnergy == 40);
        assert(rules.turnLimit == 140);
        assert(rules.moveCost == 1);
        assert(rules.roughTerrainCost == 3);
        assert(rules.waitCost == 1);
        assert(rules.rejectedMoveCost == 1);
        assert(rules.resourcePoints == 8);
        assert(rules.batteryEnergy == 2);
        assert(rules.trapEnergyPenalty == 3);
        assert(rules.trapScorePenalty == 2);
    }

    void testDefaultRulesAreStandard() {
        // Los valores por defecto de GameRules y de las celdas son los del perfil standard
        assert(GameRules{} == rulesFor(Difficulty::standard));
    }

    void testProfilesDifferFromEachOther() {
        assert(!(rulesFor(Difficulty::easy) == rulesFor(Difficulty::standard)));
        assert(!(rulesFor(Difficulty::hard) == rulesFor(Difficulty::standard)));
    }

    void testParseDifficulty() {
        assert(parseDifficulty("easy") == Difficulty::easy);
        assert(parseDifficulty("standard") == Difficulty::standard);
        assert(parseDifficulty("hard") == Difficulty::hard);
    }

    void testParseDifficultyRejectsUnknownText() {
        assert(!parseDifficulty("medium").has_value());
        assert(!parseDifficulty("").has_value());
        // La opción de línea de comandos se escribe en minúsculas
        assert(!parseDifficulty("Easy").has_value());
    }
}

int main() {
    testStandardProfile();
    testEasyProfile();
    testHardProfile();
    testDefaultRulesAreStandard();
    testProfilesDifferFromEachOther();
    testParseDifficulty();
    testParseDifficultyRejectsUnknownText();
    return 0;
}
