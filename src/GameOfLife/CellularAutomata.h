#pragma once

namespace CellularAutomata {
    struct AutomatonRules {
        int id;
        int birth;
        int survive;
    };

    enum class FirstGenerationType {
        Empty = 0,
        UniformRandom = 1,
        RadialRandom = 2,
    };
}
