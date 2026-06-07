#pragma once
#include "../src/FilterState.h"

class TestHelper
{
public:
    static void makeFilterState(
        FilterState* state,
        std::vector<std::array<double, 3>>& roots,
        float gain)
    {
        state->zeros.clear();
        state->poles.clear();
        state->gain.setValue(gain, nullptr);
        for (auto& r : roots)
            auto sr = state->add(r[0], { r[1], r[2] });
    }
};