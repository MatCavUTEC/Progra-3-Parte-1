#include "circuit_escape/scenario.hpp"

Cell cellFromSymbol(const char symbol, const GameRules& rules) {
    switch (symbol) {
        case startSymbol:
        case '.':
            return Empty{};
        case '#':
            return Wall{};
        case '~':
            return RoughTerrain{rules.roughTerrainCost};
        case 'R':
            return ResourceCell<int>{rules.resourcePoints};
        case 'B':
            return Battery{rules.batteryEnergy};
        case 'T':
            return Trap{rules.trapEnergyPenalty, rules.trapScorePenalty};
        case 'S':
            return Exit{};
        default:
            throw std::invalid_argument(std::string("cellFromSymbol: símbolo desconocido '") + symbol + "'");
    }
}
